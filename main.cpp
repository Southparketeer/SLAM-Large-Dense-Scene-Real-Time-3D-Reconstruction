#include "tsdfcuda.h"
#include <Eigen2/Dense>
#include <cmath>
#include <fstream>
#include <thread>
#include <chrono>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "StdAfx.h"
#include "Depth.h"
#include "Base.h"

#include <wrap/ply/plylib.h>
#include <wrap/ply/plystuff.h>
#include <wrap/io_trimesh/import.h>
#include <wrap/io_trimesh/export_ply.h>
#include <wrap/ply/plylib.cpp>
#include <vcg/complex/algorithms/clean.h>
#include "Timer.h"
#define DEBUG_SDF 0
#define RATIO 0.75
#define BILATERIAL 0
#define MEDIUM 1
typedef unsigned char BYTE;

#define DEPTH_RAW_WIDTH             512
#define DEPTH_RAW_HEIGHT            424
#define DEPTH_NORM_FOCAL_LENGTH_X (0.72113f)
#define DEPTH_NORM_FOCAL_LENGTH_Y (0.870799f)
#define DEPTH_NORM_PRINCIPAL_POINT_X (0.50602675f)
#define DEPTH_NORM_PRINCIPAL_POINT_Y (0.499133f)
int w = DEPTH_RAW_WIDTH;
int h = DEPTH_RAW_HEIGHT;
Timer ttime;
const int3 volres = make_int3(512,512,512);
const float trunc_dist_factor = 2.0;
loo::BoundingBox bbox(make_float3(-1000,-1000,-1000), make_float3(1000,1000 ,1000));
const loo::ImageIntrinsics K(DEPTH_NORM_FOCAL_LENGTH_X * w, DEPTH_NORM_FOCAL_LENGTH_Y * h, w /2, h /2);
loo::Image<float, loo::TargetHost, loo::Manage> h_rawdepth(w,h);
loo::Image<float, loo::TargetDevice, loo::Manage> d_tsdf_depth(w,h);
#if BILATERIAL
loo::Image<float, loo::TargetDevice, loo::Manage> d_tsdf_depth_bi(w,h);
#elif MEDIUM
loo::Image<float, loo::TargetDevice, loo::Manage> d_tsdf_depth_mi(w,h);
#endif
loo::Image<float4, loo::TargetDevice, loo::Manage> d_tsdf_normal(w,h);
loo::Image<float4, loo::TargetDevice, loo::Manage> d_tsdf_vertex(w,h);
loo::BoundedVolume<loo::SDF_t, loo::TargetDevice, loo::Manage> vol(volres.x, volres.y, volres.z, bbox);
const float trunc_dist = trunc_dist_factor*length(vol.VoxelSizeUnits());
loo::Image<float, loo::TargetDevice, loo::Manage> d_raycast_render(w, h);
loo::Image<float, loo::TargetDevice, loo::Manage> d_raycast_depth(w, h);
loo::Image<float4, loo::TargetDevice, loo::Manage> d_raycast_norm(w, h);
loo::Image<float,  loo::TargetHost, loo::Manage> h_raycast_depth(w, h);
loo::Image<float4, loo::TargetHost, loo::Manage> h_raycast_normal(w,h);
loo::Image<float,  loo::TargetHost, loo::Manage> h_raycast_render(w,h);
loo::Mat<float,3,4> T_tsdf_cw;
loo::Mat<float,3,4> T_raycast_cw;
Depth * getDepth;
GLdisplay * displayGL;
bool if_resetVolume = false;
int displayidx = 0;

typedef Eigen::Matrix<float, 4,4 ,Eigen::RowMajor> Mat4;
void setVMatrix(float3 eyepose, float3 eyecenter, float3 upvector, loo::Mat<float,3,4>& matout)
{	
	float3 Pc = eyepose;
	float3 Zc = normalize(Pc - eyecenter);
	float3 Yc = normalize(upvector);
	float3 Xc = normalize(cross(Yc, Zc));
	Yc = cross(Zc, Xc);
	Mat4 Vmatrix;
	Vmatrix.setIdentity();
	Vmatrix(0,0) = Xc.x;
	Vmatrix(0,1) = Xc.y;
	Vmatrix(0,2) = Xc.z;
	Vmatrix(1,0) = Yc.x;
	Vmatrix(1,1) = Yc.y;
	Vmatrix(1,2) = Yc.z;
	Vmatrix(2,0) = Zc.x;
	Vmatrix(2,1) = Zc.y;
	Vmatrix(2,2) = Zc.z;
	Vmatrix(0,3) = -dot(Xc, Pc);
	Vmatrix(1,3) = -dot(Yc, Pc);
	Vmatrix(2,3) = -dot(Zc, Pc);
	std::cout<<Vmatrix<<std::endl<<std::endl;
	Eigen::Matrix<float,3,4, Eigen::RowMajor> Vmat = Vmatrix.inverse().block<3,4>(0,0);
	for(int r = 0 ; r < 3 ; r ++)
	{
		for(int c = 0 ; c < 4 ; c++)
		{
			matout(r,c) = Vmat(r,c);
		}
	}
	std::cout<<Vmat<<std::endl;
}

void timer( int value ) 
{	
	glutPostRedisplay();
	glutTimerFunc( 33, timer, 0 );
}
void MeshOutput();
void keyboard( unsigned char key, int x, int y ) 
{

	switch (key) 
	{
	case 27:
		exit(0);
		break;
	case 9:
		if_resetVolume = true;
		glutPostRedisplay();
		break;
	case 13:
		MeshOutput();
		break;
	}
}

void tsdfprocessing()
{
	float raycast_near = 1000;
	float raycast_far = 3000;
	float depth_clip_near = 1000;
	float depth_clip_far = 3000;
	float bigs = 3, bigr = 10;
	int biwin = 2;
	float max_w = 1000;
	float mincostheta = 0.1;
	bool subpix = true;

	//TSDF FUSION
	d_tsdf_depth.CopyFrom(h_rawdepth);
#if BILATERIAL
	loo::BilateralFilter<float, float>(d_tsdf_depth_bi, d_tsdf_depth, bigs, bigr, biwin);
	loo::DepthToVbo<float>(d_tsdf_vertex, d_tsdf_depth_bi, K);
#elif MEDIUM 
	loo::MediumFilter<float, float>(d_tsdf_depth_mi, d_tsdf_depth, biwin, depth_clip_near, depth_clip_far);
	loo::DepthToVbo<float>(d_tsdf_vertex, d_tsdf_depth_mi, K);
#else 
	loo::DepthToVbo<float>(d_tsdf_vertex, d_tsdf_depth, K);
#endif
	loo::NormalsFromVbo(d_tsdf_normal, d_tsdf_vertex);
#if MEDIUM 
	loo::SdfFuse(vol, d_tsdf_depth_mi, d_tsdf_normal, T_tsdf_cw, K, trunc_dist, max_w, mincostheta);
#else
	loo::SdfFuse(vol, d_tsdf_depth, d_tsdf_normal, T_tsdf_cw, K, trunc_dist, max_w, mincostheta);
#endif

	//RAYCASTING
	loo::RaycastSdf(d_raycast_depth, d_raycast_norm, d_raycast_render, vol, T_raycast_cw, K, raycast_near, raycast_far, 0, subpix );
	h_raycast_depth.CopyFrom(d_raycast_depth);
	h_raycast_normal.CopyFrom(d_raycast_norm);
	h_raycast_render.CopyFrom(d_raycast_render);
}

void render()
{
	if(if_resetVolume) 
	{
		loo::SdfReset(vol, trunc_dist);
		if_resetVolume = false;
	}
	getDepth->Update(h_rawdepth.ptr, 1000, 3000);
	tsdfprocessing();
	displayGL->display(h_rawdepth.ptr, h_raycast_normal.ptr, h_raycast_render.ptr);
}

void MeshOutput()
{
	std::vector<float3> verts;
	std::vector<int3> faces;
	ttime.Start();
	loo::SaveMesh("volume.ply", vol, verts, faces, trunc_dist);
	ttime.Print("MC");
	vcg::CMeshO cm;
	cm.vert.resize(verts.size());
	cm.vn = cm.vert.size();
	cm.face.resize(faces.size());
	cm.fn = cm.face.size();
	int a = cm.vert.size();
	int b = 0;

	ttime.Start();
	std::cout<<cm.vert.size()<<std::endl;
	Concurrency::parallel_for(0, (int) cm.vert.size(), [&](int k)
	{
		cm.vert.at(k).P() = vcg::Point3f(verts[k].x, verts[k].y, verts[k].z);
	});

	CVertexO * V_start = &cm.vert[0];

	for(int k = 0 ; k < cm.face.size(); k++)
	{
		cm.face[k].setvptr(0, V_start + faces[k].x);
		cm.face[k].setvptr(2, V_start + faces[k].y);
		cm.face[k].setvptr(1, V_start + faces[k].z);
	}
	//Mesh Clean by VCG 
	tri::UpdateNormal<CMeshO>::PerVertex(cm);
	tri::UpdateNormal<CMeshO>::NormalizePerVertex(cm);
	int dup = tri::Clean<CMeshO>::RemoveDuplicateVertex(cm);
	int cls = tri::Clean<CMeshO>::MergeCloseVertex(cm, 2);
	int unref = vcg::tri::Clean<CMeshO>::RemoveUnreferencedVertex(cm);
	printf("1. Remove %d duplicate Vertex\n2. Merge %d Vertex\n3. Remove %d unreference vertex\n", dup, cls, unref);

	CMeshO cm2;
	tri::Append<CMeshO, CMeshO>::MeshCopy(cm2, cm);

	const int minCC= (int)(cm2.vert.size() * 0.01f);
	tri::UpdateTopology<CMeshO>::FaceFace(cm2);
	std::pair<int,int> delInfo=tri::Clean<CMeshO>::RemoveSmallConnectedComponentsSize(cm2,minCC);
	printf("4. remove %d component out of %d component\n", delInfo.first, delInfo.second);
	tri::io::ExporterPLY<CMeshO>::Save(cm2, "tsdfply.ply", vcg::tri::io::Mask::IOM_VERTNORMAL, true);
	ttime.Print("vcg");
}

int main(int argc, char** argv )
{
	setVMatrix(make_float3(0,0,-2000), make_float3(0,0,0), make_float3(0,1,0), T_tsdf_cw);
	T_raycast_cw = T_tsdf_cw;
	T_raycast_cw (2,2) *= -1;
	loo::SdfReset(vol, trunc_dist);
	getDepth = new Depth();
	getDepth->InitializeDefaultSensor();

	std::cout<<"\n----------thread sleep----------\n"<<std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout<<"\n----------thread weakup----------\n"<<std::endl;

	displayGL = new GLdisplay(DEPTH_RAW_WIDTH, DEPTH_RAW_HEIGHT);
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB |GLUT_DEPTH );
	glutInitWindowSize( displayGL ->gl_Windows_Width * RATIO, displayGL -> gl_Windows_Height * RATIO); 
	glutInitWindowPosition( 100, 100 );
	glutCreateWindow( argv[0] );

	glewInit();
	displayGL->init();
	glutDisplayFunc( render );
	glutKeyboardFunc( keyboard );
	glutTimerFunc(16, timer, 0);
	glutMainLoop();

	delete getDepth;
	getDepth = NULL;
	return 0;
}