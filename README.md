# Adaptive GPU Tessellation with Compute Shaders 

[![Build Status](https://travis-ci.org/jadkhoury/TessellationDemo.svg?branch=master)](https://travis-ci.org/jadkhoury/TessellationDemo) [![Build status](https://ci.appveyor.com/api/projects/status/445xn4iwo7p280yw?svg=true)](https://ci.appveyor.com/project/jadkhoury/tessellationdemo)


This repository contains the demo code for the research project Tessellation On Compute Shader.
Requires OpenGL 4.5 min. 
Do not hesitate to contact me if you have any feedbacks, problems, remarks or question about the code, by contacting me at jad.n.ky@gmail.com

# Cloning

Clone the repository and all its submodules using the following command:
```sh
git clone git clone --recurse-submodules https://github.com/jadkhoury/TessellationDemo.git
```

If you accidentally omitted the `recurse-submodules` flag when cloning the repository you can retrieve the submodules like so:
```sh
git submodule update --init --recursive
```

# Running
Enter the following commands in the terminal
```sh
cd TessellationDemo
mkdir build
cd build
cmake ../
make
./demo 
or
./demo <.obj file name>
or 
./bench
```

# Compute Tess Project
The Bench subproject contains more or less the code from the demo, minus some late refratoring, and including some code measuring and outputting the performances of our pipeline in a Zoom-Dezoom setup.
```
├── CMakeLists.txt
├── common
│   ├── glad
│   │   └── ...
│   ├── imgui_impl
│   │   └── ...
│   ├── objs
│   │   ├── bigguy.obj
│   │   ├── bunny.obj
│   │   ├── cube.obj
│   │   ├── cube_quads.obj
│   │   ├── distancebased_regular.obj
│   │   ├── distancebased_warp.obj
│   │   ├── tangle_cube.obj
│   │   └── triangle.obj
│   └── submodules
│       └── ...
├── ComputeTess_bench
│   └── ...
├── ComputeTess_demo
│   ├── bintree.h
│   ├── commands.h
│   ├── common.h
│   ├── main.cpp
│   ├── mesh.h
│   ├── mesh_utils.h
│   ├── shaders
│   │   ├── bintree_compute.glsl
│   │   ├── bintree_copy.glsl
│   │   ├── bintree_render_common.glsl
│   │   ├── bintree_render_flat.glsl
│   │   ├── bintree_render_wireframe.glsl
│   │   ├── gpu_noise_lib.glsl
│   │   ├── LoD.glsl
│   │   ├── ltree_jk.glsl
│   │   ├── noise.glsl
│   │   ├── phong_interpolation.glsl
│   │   └── PN_interpolation.glsl
│   ├── transform.h
│   └── utility.h
└── README.md

```
## The GUI & Input
### Trackball model for the mouse: 
* left-click + drag for camera rotation
* right-click + drag for camera panning
* Mouse wheel to advance on camera line of sight

### GUI:
* Mode: switch between terrain and mesh
* Advanced Mode: toggles additional GUI controls
* Render Projection: toggles the MVP Matrix
* FoV: Controls field of view of camera
* Reinit Cam: reinitialize the camera for current mode
* Wireframe: toggles the solid wireframe shading
* Flat Normal: toggles the flat normal computation in fragment shader, instead of procedural displaced normals (better performances and tessellation visualization)
* Displacement Mapping: Toggles the dislacement of the flat grid (TERRAIN mode only)
* Height factor: manipulates the height of the displacement map
* Rotate Mesh: rotates the mesh around the z axis
* Uniform: toggle uniform subdivision (with slider for level)
* Edge Length: slider for the target edge length, in px, as power of two (4 on the slider = 2^4 = 8px)
* Readback Node Count: Readbacks the number of nodes in the bintree, and the total number of rendered triangles (after culling). Slightly affect performances.
* Polygon Type: Switch between Triangles and Quads (TERRAIN mode only, auto defined for mesh)
* CPU LoD: Level of subdivision of the instanced triangle grid
* Interpolation type: Switch between linear, PN and Phong interpolation (MESH mode only)
* The rest is self-explanatory

## The Code

### C++ files

#### `bintree.h`:
Class containing the CPU side of the main work of this project: the bintree algorithm.
* Defines the `Settings` struct, containing all the parfameters of the bintrees, accessed by the GUI
* Manages the 3 OpenGL programs corresponding to the 3 Passes to render one frame: 
    * `compute_program_`: implemented in `bintree_compute.glsl`
    * `copy_program_`: implemented in `bintree_copy.glsl`
    * `render_program_`: implemented in `bintree_render_*.glsl`
    * More information in the code
* Manages the 3 buffers containing the nodes of the bintrees: 
    * Complete Bintree at frame t-1 (read)
    * Complete Bintree at frame t (write)
    * Culled Bintree at frame t (write)
* Generates the leaf geometry (aka the instanced triangle grid), and manages the buffers and vertex arrays storing them
* Recieves as parameter at initialization a pointer to the `Mesh_Data` structure containing all data for the mesh, a pointer to the uniform buffer containing the transforms (managed by the `TransformsManager` in `transform.h`), as well as a set of initialization settings stored in a `BinTree::Settings` object.
* Draws the mesh using our bintree implementation, by first updating the bintree once using the `compute_program_`, then copying the relevant parameters in the indirect command buffers in the `copy_program_`, and finally drawing the mesh in the `render_program_`

#### `commands.h`:  
* Manages and binds the atomic counters keeping track of the number of primitives to instanciate. 
* Manages and binds the indirect Draw and Dispatch buffers

#### `common.h`: 
Header file containging all the using & includes used across the project, along with: 
* `struct BufferData`: Represents a buffer
* `struct BufferCombo`: Usefull data structure to manage geometry buffers 
* `struct Vertex`: // Data structure representing a unique vertex as a set of a
    * 3D position
    * 3D normal
    * 2D UV coordinate
* `struct Mesh_Data`: Stores all data necessary to represent a mesh

####  `mesh_utils.h`: 
Namespace for generating and managing meshes (grids, obj parsing and storing in mesh_data...)

#### `mesh.h`: 
* Class allowing the opaque use of our bintree algorithm for mesh rendering
* Relays the camera and frustum settings to the Transforms Manager
* Holds instances of the Bintree as well as the Transforms Manager

####  `transform.h`: 
Header file containing both the `CameraManager` and the `TransformsManager`.
The `CameraManager` is responsible for treating the mouse input and updating the current camera setup accordingly.
The `TransformsManager` then updates the transforms matrices accordingly, and pushes them in an uniform buffer, accessed all along our render pipeline.

#### `utility.h`:
Header containing the utility namespace for small usefull functions used across the project, as well as some OpenGL debug callbacks

#### `main.cpp`
* `OpenGLApp` struct that stores a few render settings, the default filepath, some mouse related temp variables, the light position, and instances of the Mesh class and the Camera Manager class
* `BenchStats` struct that contains and updates the benchmarking variables
* Displays the GUI using the `imgui` API
* Manages the mouse input, relay them to the `CameraManager`, notify the `Mesh` class accordingly (In particular: calls the relevant functions of the Camera Manager, notifies the Mesh class that it has to update the transforms, which then in turn relays the calls to the Transform Manager that updates the matrices)
* Manages the resize callbacks.
* Launch and manages the render loop
* Handles arguments passed at the call of the program (`.obj` file path)


### GLSL Shaders

#### `bintree_compute.glsl`
Holds the compute pass of the render pipeline, that updates the bintree data structure and performs the frustum culling

#### `bintree_copy.glsl`
Holds the BatcherKernel program, in charge of preparing the indirect draw command buffer for the current pass, and the dispatch indirect command buffer for the compute pass of the next pass

#### `bintree_render_common.glsl`
Holds a few function common to both the standard render program and the wireframe render program

#### `bintree_render_flat.glsl`
Holds the standard render program: 
* Vertex Shader: takes as input the vertex of the instanced triangle grid to render, reads a key from the bintree data structure, compute the vertex bintree space position, interpolates in world space position using the relevant interpolation method (either Linear, Phong, or Curved PN Triangles). If displacement mapping is activated, performs the displacement
* Fragment Shader: use the world space position of the vertex to perform the shading. If displacement mapping is activated and flat normal is not, computes the vertex normal as gradient of the procedural displacement map

#### `bintree_render_wireframe.glsl`
Similar to the `bintree_render_flat`, with the addition of a Geometry stage that allows us to use solid wireframe.

#### `gpu_noise_lib.glsl`
GLSL library to generate procedural noise, used in our heightmap generation

#### `LoD.glsl`
Contains functions relative to the distance based LoD computation and culling. Also defines the Transforms uniform buffer, used accross the shaders

#### `ltree_jk.glsl`
My own implementation of the bintree management functions (key generation for parent/children, level evaluation, mapping from one space to another). The keys are implemented as ulong int, simulated as a uvec2 concatenation, allowing 63 levels of subdivision.

#### `noise.glsl`
Contains function for the procedural heightmap computation, relying on gpu_noise_lib

#### `Phong.glsl`
Performs Phong interpolation on the current Vertex instance by using the normals, uv and coordinates of the currently rendered mesh polygon.

#### `PN.glsl`
Similar to `Phong.glsl`, but for the Curved PN Triangles method.


