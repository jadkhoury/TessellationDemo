# Compute Shader Surface Subdivision

This repository contains the demo code for the research project Tessellation On Compute Shader.

# Cloning

Clone the repository and all its submodules using the following command:
```sh
git clone --recursive https://github.com/jadkhoury/TessellationDemo.git
```

If you accidentally omitted the `--recursive` flag when cloning the repository you can retrieve the submodules like so:
```sh
git submodule update --init --recursive
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
    * 
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
    * Complete Quadtree at frame0 t-1 (read)
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

