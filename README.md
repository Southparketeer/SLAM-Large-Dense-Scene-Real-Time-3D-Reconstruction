# SLAM Large Dense Scene Real-time 3D Reconstruction
GPU based camera tracking volumetric fusion and rendering.
* Camera Tracking: Track pose in real-time by registering incoming depth image to the TSDF-generated depth map. 
* Volumetric Fusion: With the updated camera pose, fuse the new income depth to the fused TSDF volume in real-time. 
* Volumetric Rendering: Render the fused TSDF volume in real-time.

## GPU Based Real-time RGBD Camera Tracking
### Rigid Iterative Closest Point (ICP) with Point-to-plane Error Metric
To estimate the 6 DoF for camera pose update (3 for rotation and 3 for translation), we use rigid registration algorithm - ICP. The objective function is shown as follow, where s denotes the source vertex position; d denotes its corresponding target vertex position; n denotes the surface normal on target vertex d. [1] The advantage of point-to-plane error metric compared with the point-to-point error metric is the point-to-plane error metric allows two surface to slide over each other, and thus to avoid being trapped into undesired local minima. Comparison of the two can be found in [2].

``` 
For each iteration:
1) We define correspondences projective correspondence (see Projective Correspondence).
2) We derive the optimal transformation T by solving the objective function.
3) We update the pose of source mesh using the optimized T
```

<p align="center">
<img width="400" src= http://latex.codecogs.com/gif.latex?%5Cbold%7BT%7D_%7Bopt%7D%20%3D%20%5Carg%20min_%7B%5Cbold%7BT%7D%7D%5Csum_i%20%7B%5Cleft%20%5C%7C%28%5Cbold%7BT%7D%20%5Ccdot%20%5Cbold%7Bs%7D_%7Bi%7D%20-%20%5Cbold%7Bd%7D_%7Bi%7D%29%5Ccdot%20%5Cbold%7Bn%7D_i%20%7D%5C%7C_2>
</p>

<p align="center">
<img width="400" src= demo/explain_ptp.png>
</p>

### Projective Correspondences

Incoming depth image
<p align="center">
   <img width="400" src= demo/incoming_depth.png>
</p>

Generated depth image from TSDF volume

## GPU Based Real-time Volumetric Fusion

## GPU Based Real-time Volumetric Rendering with Ray Casting

## Demo
### User Interface
![UI](demo/SLAM_UI.png)
### Output Reconstructed 3D Mesh
![Mesh_Output](demo/SLAM_MeshOutput_1.png)
### More Examples
![More Example](demo/raycasting.png)

## Install
dependency: 
1. VCG library http://vcg.isti.cnr.it/vcglib/ for geometry processing
2. Eigen http://eigen.tuxfamily.org/index.php?title=Main_Page for matrix computation
3. CUDA https://developer.nvidia.com/cuda-downloads for CUDA kernel
4. OpenGL https://www.opengl.org/ for display
5. Microsoft Kinect SDK https://www.microsoft.com/en-us/download/details.aspx?id=44561 
   or libfreenect2 https://github.com/OpenKinect/libfreenect2 for depth strame input from Kinect
   
## Reference 
[1] Low, Kok-Lim. "Linear least-squares optimization for point-to-plane icp surface registration." Chapel Hill, University of North Carolina 4, no. 10 (2004): 1-3.
[2] Rusinkiewicz, Szymon, and Marc Levoy. "Efficient variants of the ICP algorithm." In 3dim, vol. 1, pp. 145-152. 2001.



