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
#include "png_io.h"

#define DEBUG_SDF 0
#define RATIO 0.75
#define BILATERIAL 1
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
#endif
#if MEDIUM
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
GLdisplay * displayGL;
bool if_resetVolume = false;
float max_w = 1000;

typedef Eigen::Matrix<float, 4,4 ,Eigen::RowMajor> Mat4;
void setVMatrix(float3 eyepose, float3 eyecenter, float3 upvector, loo::Mat<float,3,4>& matout)	{	
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
	for(int r = 0 ; r < 3 ; r ++) {
		for(int c = 0 ; c < 4 ; c++) {
			matout(r,c) = Vmat(r,c);
		}
	}
	std::cout<<Vmat<<std::endl;
}

void timer( int value ) {	
	glutPostRedisplay();
	glutTimerFunc( 33, timer, 0 );
}
void MeshOutput();
void TsdfOutput();
void keyboard( unsigned char key, int x, int y ) {

	switch (key) {
	case 27:
		exit(0);
		break;
	case 9:
		if_resetVolume = true;
		glutPostRedisplay();
		break;
	case 13:
		MeshOutput();
		//TsdfOutput();
		break;
	}
}

void tsdfprocessing() {
	float raycast_near = 1000;
	float raycast_far = 3000;
	float depth_clip_near = 1000;
	float depth_clip_far = 3000;
	float bigs = 1.5, bigr = 0.01;
	int biwin = 2;

	float mincostheta = 0.1;
	bool subpix = true;

	//TSDF FUSION
	d_tsdf_depth.CopyFrom(h_rawdepth);
//#if BILATERIAL
//	loo::BilateralFilter<float, float>(d_tsdf_depth_bi, d_tsdf_depth, bigs, bigr, biwin);
//	loo::DepthToVbo<float>(d_tsdf_vertex, d_tsdf_depth_bi, K);
//#elif MEDIUM 
//	loo::MediumFilter<float, float>(d_tsdf_depth_mi, d_tsdf_depth, biwin, depth_clip_near, depth_clip_far);
//	loo::DepthToVbo<float>(d_tsdf_vertex, d_tsdf_depth_mi, K);
//#else 
//	loo::DepthToVbo<float>(d_tsdf_vertex, d_tsdf_depth, K);
//#endif
//	loo::NormalsFromVbo(d_tsdf_normal, d_tsdf_vertex);
//#if MEDIUM 
//	loo::SdfFuse(vol, d_tsdf_depth_mi, d_tsdf_normal, T_tsdf_cw, K, trunc_dist, max_w, mincostheta);
//#else
//	loo::SdfFuse(vol, d_tsdf_depth, d_tsdf_normal, T_tsdf_cw, K, trunc_dist, max_w, mincostheta);
//#endif
	loo::MediumFilter<float, float>(d_tsdf_depth_mi, d_tsdf_depth, biwin, depth_clip_near, depth_clip_far);
	loo::BilateralFilter<float, float>(d_tsdf_depth_bi, d_tsdf_depth_mi, bigs, bigr, biwin);
	loo::DepthToVbo<float>(d_tsdf_vertex, d_tsdf_depth_bi, K);
	loo::NormalsFromVbo(d_tsdf_normal, d_tsdf_vertex);
	loo::SdfFuse(vol, d_tsdf_depth_bi, d_tsdf_normal, T_tsdf_cw, K, trunc_dist, max_w, mincostheta);
	//RAYCASTING
	loo::RaycastSdf(d_raycast_depth, d_raycast_norm, d_raycast_render, vol, T_raycast_cw, K, raycast_near, raycast_far, 0, subpix );
	h_raycast_depth.CopyFrom(d_raycast_depth);
	h_raycast_normal.CopyFrom(d_raycast_norm);
	h_raycast_render.CopyFrom(d_raycast_render);
}

//void render() {
//	if(if_resetVolume) {
//		loo::SdfReset(vol, trunc_dist);
//		if_resetVolume = false;
//	}
//	getDepth->Update(h_rawdepth.ptr, 1000, 3000);
//	tsdfprocessing();
//	displayGL->display(h_rawdepth.ptr, h_raycast_normal.ptr, h_raycast_render.ptr);
//}

#define FRAME_NUM 60
int global_index = 0;
vector<vector<float>> depth_buff(FRAME_NUM);


void render2() {
	cout<<global_index<<" ";
	if(if_resetVolume) {
		loo::SdfReset(vol, trunc_dist);
		if_resetVolume = false;
	}
	memcpy(h_rawdepth.ptr, depth_buff.at(global_index).data(), DEPTH_RAW_HEIGHT * DEPTH_RAW_WIDTH * sizeof(float));
	tsdfprocessing();
	displayGL->display(h_rawdepth.ptr, h_raycast_normal.ptr, h_raycast_render.ptr);
	global_index ++;
	global_index %= FRAME_NUM;
}

inline int countones(unsigned char c) {
	int count = 0;
	for(int i = 0 ; i < 8 ; i ++) {
		if(c & 1 << i ) count ++;
	}
	return count;
}

void MeshClean(CMeshO & cm_in, CMeshO & cm_out) {
	tri::UpdateNormal<CMeshO>::PerVertex(cm_in);
	tri::UpdateNormal<CMeshO>::NormalizePerVertex(cm_in);
	int dup = tri::Clean<CMeshO>::RemoveDuplicateVertex(cm_in);
	int cls = tri::Clean<CMeshO>::MergeCloseVertex(cm_in, 2);
	int unref = vcg::tri::Clean<CMeshO>::RemoveUnreferencedVertex(cm_in);
	printf("Remove %d duplicate Vertex\n2. Merge %d Vertex\n3. Remove %d unreference vertex\n", dup, cls, unref);

	tri::Append<CMeshO, CMeshO>::MeshCopy(cm_out, cm_in);
	const int minCC= (int)(cm_out.vert.size() * 0.1f);
	tri::UpdateTopology<CMeshO>::FaceFace(cm_out);
	std::pair<int,int> delInfo=tri::Clean<CMeshO>::RemoveSmallConnectedComponentsSize(cm_out,minCC);
	int unref2 = vcg::tri::Clean<CMeshO>::RemoveUnreferencedVertex(cm_in);
	printf("remove %d component out of %d component\n", delInfo.first, delInfo.second);
}


void MeshOutput() {
	int abcd;
	abcd = 0;
	ttime.Start();
	loo::BoundedVolume<uchar2, loo::TargetDevice, loo::Manage> d_vol_mask(volres.x, volres.y, volres.z, bbox);
	loo::MarchingCubeMaskGPU(vol, d_vol_mask, trunc_dist);
	loo::BoundedVolume<uchar2, loo::TargetHost, loo::Manage> h_vol_mask(volres.x, volres.y, volres.z, bbox);
	h_vol_mask.CopyFrom(d_vol_mask);
	vector<uchar2> mask;
	vector<int3> index;
	int count_triangles = 0;
	mask.reserve(200000);
	index.reserve(200000);
	for( int z = 0 ; z < volres.z -1 ; z++ ) {
		for( int y = 0 ; y < volres.y -1 ; y++ ) {
			for(int x = 0 ; x < volres.x -1 ; x++ )	{
				uchar2 m = h_vol_mask(x,y,z);
				if(m.x != 0 && m.y != 0) {
					mask.push_back(m);
					index.push_back(make_int3(x,y,z));
					count_triangles += countones(m.y);
				}
			}
		}
	}

	vector<float3> verts;
	vector<int3> faces;
	verts.clear();
	faces.clear();
	verts.reserve(3 * count_triangles);
	faces.reserve(count_triangles);

	loo::BoundedVolume<loo::SDF_t, loo::TargetHost, loo::Manage> h_vol(volres.x, volres.y, volres.z, bbox);
	h_vol.CopyFrom(vol);
	loo::SaveMesh2(h_vol, verts, faces, index, mask);

	ttime.Print("Marching Cubes...");

	ttime.Start();
	vcg::CMeshO cm;
	cm.vert.resize(verts.size());
	cm.vn = cm.vert.size();
	cm.face.resize(faces.size());
	cm.fn = cm.face.size();

	Concurrency::parallel_for(0, (int) cm.vert.size(), [&](int k) {
		cm.vert.at(k).P() = vcg::Point3f(verts[k].x, verts[k].y, verts[k].z);
	});

	CVertexO * V_start = &cm.vert[0];

	for(int k = 0 ; k < cm.face.size(); k++) {
		cm.face[k].setvptr(0, V_start + faces[k].x);
		cm.face[k].setvptr(2, V_start + faces[k].y);
		cm.face[k].setvptr(1, V_start + faces[k].z);
	}
	CMeshO cm_clean;
	MeshClean(cm, cm_clean);
	ttime.Print("Mesh Clean...");

	ttime.Start();
	tri::io::ExporterPLY<CMeshO>::Save(cm_clean, "tsdfply.ply", vcg::tri::io::Mask::IOM_VERTNORMAL, true);
	ttime.Print("Mesh Output...");
	if_resetVolume = true;
}




int main(int argc, char** argv ) {
	//input depth here for testing
	//depth_buff store the readin depth images
	//FRAME_NUM is how many frames you import
	for(int i = 0 ; i < FRAME_NUM ; i++ ) {
		string s("depth//pose5//");
		s.append(to_string(i));
		s.append("_Depth.png");
		depth_buff.at(i).resize(DEPTH_RAW_WIDTH * DEPTH_RAW_HEIGHT);
		PNG_IO::PNG_to_array_depth(s.c_str(), depth_buff[i].data(), DEPTH_RAW_WIDTH, DEPTH_RAW_HEIGHT);
		cout<<"import depth image "<<i<<endl;
	}

	setVMatrix(make_float3(0,0,-2000), make_float3(0,0,0), make_float3(0,1,0), T_tsdf_cw);
	T_raycast_cw = T_tsdf_cw;
	T_raycast_cw (2,2) *= -1;
	loo::SdfReset(vol, trunc_dist);

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
	glutDisplayFunc( render2 );
	glutKeyboardFunc( keyboard );
	glutTimerFunc(16, timer, 0);
	glutMainLoop();

	//delete getDepth;
	//getDepth = NULL;
	return 0;
}