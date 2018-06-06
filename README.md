# Compute Shader Surface Subdivision

This repository contains the demo code for the research project Tessellation On Compute Shader.
Requires OpenGL 4.5 min.

# Cloning

Clone the repository and all its submodules using the following command:
```sh
git clone --recursive https://github.com/jadkhoury/TessellationDemo.git
```

If you accidentally omitted the `--recursive` flag when cloning the repository you can retrieve the submodules like so:
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
./new_distance 
or
./new_distance <.obj file name>
```



# 2_NewDistance
Only the subproject `2_NewDistance` is fully functoinnal, other implementation are still in progress.
(in progress)
```
TessellactionDemo/
├── 0_FullProgram
│   └── ...
├── 1_DistancePipeline
│   └── ...
├── 2_NewDistance
│   ├── commands.h
│   ├── common.h
│   ├── main.cpp
│   ├── mesh_utils.h
│   ├── quadtree.h
│   ├── shaders
│   │   ├── dj_frustum.glsl
│   │   ├── dj_heightmap.glsl
│   │   ├── gpu_noise_lib.glsl
│   │   ├── LoD.glsl
│   │   ├── ltree_jk.glsl
│   │   ├── quadtree_compute.glsl
│   │   ├── quadtree_copy.glsl
│   │   └── quadtree_render.glsl
│   └── utility.h
├── CMakeLists.txt
├── common
│   ├── glad
│   ├── imgui_impl
│   ├── objs
│   └── submodules
│       └── ...
└── README.md
```
## The GUI & imput
### Trackball model for the mouse: 
* left-click + drag for camera rotation
* right-click + drag for camera panning
* Mouse wheel to advance on camera line of sight

### GUI:
* Mode: switch between terrain and mesh
* Render Projection: toggles the MVP Matrix
* FoV: Controls field of view of camera
* Reinit Cam: Reinitialize the camera for current mode
* Rotate Mesh: 
* Color Mode: Switch between different rendering mode highlighting different properties of the quadtree algorithm
* Uniform: Toggle uniform subdivision (with slider for level)
* LoD Factor: Factor of the adaptive subdivision
* Readback Primitive Count: Readbacks the number of primitive instanced 
* Root Primitive: Switch between Triangles and Quads in terrain mode (auto switch for mesh)
* CPU LoD: Level of subdivision of the instanced mesh (cannot be less than 2 for morphing in distance tessellation)
* The rest is self-explanatory

## The Code

### `TL;DR`
For the reader, the sections of interest are 
* `quadtree.h`
* `Mesh` data structure and functions in `main.cpp`
* eventually `common.h` for data struct references & includes used.
To use a custom mesh, use the full path as argument of the executable file, *e.g.* `./new_distance /home/person/path/to/example.obj`


### `commands.h`:  
* Manages and binds the atomic counter array keeping track of the number of primitives to instanciate. 
* Manages and binds the indirect Draw and Dispatch buffers

### `common.h`: 
header file containging all the using & includes used across the project: 
* `struct BufferData`: Represents a buffer
* `struct BufferCombo`: Usefull data structure to manage geometry buffers 
* `struct Vertex`: // Data structure representing a unique vertex as a set of a
    * 3D position
    * 3D normal
    * 2D UV coordinate
* `struct Mesh_Data`: Stores all data necessary to represent a mesh
* `struct Transfo`:  Nice little struct to manage transforms

###  `mesh_utils.h`: 
Namespace for generating and managing meshes

### `utility.h`:
Header containing the utility namespace containing small usefull functions used across the project, as well as some OpenGL debug callbacks

### `quadtree.h`:
Class containing the CPU side of the main work of this project: the quadtree algorithm.
* Defines the `Settings` struct, containing all the parfameters of the quadtrees, accessed by the GUI
* Manages the 3 OpenGL programs corresponding to the 3 Passes to render one frame: 
    * `compute_program_`: implemented in `quadtree_compute.glsl`
    * `copy_program_`: implemented in `quadtree_copy.glsl`
    * `render_program_`: implemented in `quadtree_render.glsl`
    * More information in the code
* Manages the 3 buffers containing the nodes of the quadtrees: 
    * Complete Quadtree at frame t-1 (read)
    * Complete Quadtree at frame t (write)
    * Culled Quadtree at frame t (write)
* Generate the leaf geometry (aka the instanced triangle grid), and manages the buffers and vertex arrays storing them
* Recieve as parameter at initialization a pointer to the `Mesh_Data` structure containing all data for the mesh, a pointer to the `Transforms` data struct that contain and manages the transform matrices for the mesh rendering, as well as a set of initialization settings store in a `QuadTree::Settings` object.
* Draws the mesh using our quadtree implementation, by first updating the quadtree once using the `compute_program_` (i.e. 1 max subdivision / merge per frame), then copying the relevant parameters in the indirect command buffer in the `copy_program_`, and finally draw the mesh in the `render_program_`

### `main.cpp`
* `Mesh` struct: manages the mesh itself. 
    * To change the density of the terrain grid, change the `grid_quads_count` variable
    * To change the .obj represented as a quadtree, change the `filepath` variable and put your .obj file in `TessellactionDemo/common/objs/`
* `BenchStats` struct: contains and updates the benchmarking variables
* `CameraManager`: represents the current camera state, used to update the `Transforms` instance of the `Mesh` pased to the `QuadTree`  
* Displays the GUI using the `imgui` API
* Stores the input callbacks.

### `dj_frustum.glsl`
J. Dupuy's shader API for culling

### `dj_heightmap.glsl`
J. Dupuy's shader API for heightmapping

### `gpu_noise_lib.glsl`
Well known shader API for GPU noise generation

### `LoD.glsl`
Shader containing the LoD functions used to subdivide the quadtree, and used during the compute pass (`quadtree_compute.glsl`) 

### `ltree_jk.glsl`
My own implementation of the quadtree management functions (key generation for parent/children, level evaluation, mapping from one space to another)