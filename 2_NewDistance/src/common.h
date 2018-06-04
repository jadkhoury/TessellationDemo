#ifndef COMMON_H
#define COMMON_H

////////////////////////////////////////////////////////////////////////////////
///
/// Some types and libs used across the project
///

#include <iostream>
#include <vector>
#include <iterator>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl.h"
#include "utility.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/component_wise.hpp>

#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include "stb_image_write.h"

#define DJ_OPENGL_IMPLEMENTATION 1
#include "dj_opengl.h"
#define DJ_ALGEBRA_IMPLEMENTATION 1
#include "dj_algebra.h"

const char* shader_dir = "../2_NewDistance/src/shaders/";

using namespace std;
using std::string;
using std::vector;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec3;
using glm::uvec4;
using glm::mat3;
using glm::mat4;

using namespace std;

enum { TERRAIN,
       MESH,
       MODES_COUNT
     } RenderModes;

enum { LOD,
       WHITE_WIREFRAME,
       PRIMITIVES,
       FRUSTUM,
       CULL,
       DEBUG,
       COLOR_MODES_COUNT
     } ColorModes;

enum { PN,
       PHONG,
       NONE,
       INTERPOLATION_COUNT
     } InterpolationModes;

enum {  NEIGHBOUR,
        HYBRID_PRE,
        HYBRID_POST,
        MORPH_MODES_COUNT
     } MorphModes;

enum { TRIANGLES,
       QUADS,
       NUM_TYPES
     } PrimTypes;


// Represents a buffer
struct BufferData {
    GLuint bo;        // buffer object
    GLuint count = 0; // Number of elements in the buffer
    GLsizei size;     // Size of the buffer (in bytes)
};

// Usefull data structure to manage geometry buffers
struct BufferCombo {
    BufferData v;   // Buffer containing the vertices
    BufferData idx; // Buffer containing the indices
    GLuint vao;     // Vertex Array Object
};


// Data structure representing a unique vertex as a set of 
// A position
// A normal
// A UV
// Plus some filling data to align on the std430 standard
struct Vertex
{
    vec4 p  = vec4(0);
    vec4 n  = vec4(0);
    vec2 uv = vec2(0);
    vec2 align = vec2(0);

    string to_string()
    {
        string s = "";
        s += "P :  " + glm::to_string(p)   + "\n";
        s += "N :  " + glm::to_string(n)   + "\n";
        s += "UV:  " + glm::to_string(uv)  + "\n";
        return s;
    }

    Vertex(vec4 p, vec4 n)
    {
        this->p = p;
        this->n = n;
    }

    Vertex(vec4 p, vec4 n, vec2 uv)
    {
        this->p = p;
        this->n = n;
        this->uv = uv;
    }

    Vertex() {}
};

// Stores all data necessary to represent a mesh
struct Mesh_Data
{
    Vertex* v_array;   // Array of vertices
    uint* q_idx_array; // Array of indices for quad meshes
    uint* t_idx_array; // Array of indices for triangle meshes

    BufferData v, q_idx, t_idx; // Buffers for vertices, quad indices, triangle indices
    int triangle_count = 0, quad_count = 0;
};

struct Settings
{
    int uni_lvl = 0;                    // Level of uniform subdivision
    int adaptive_factor = 1;            // Factor scaling the adaptive subdivision
    bool uniform = false;                // Toggle uniform subdivision
    bool map_primcount = true;          // Toggle the readback of the node counters
    bool rotateMesh = false;            // Toggle mesh rotation (for mesh)
    bool displace = false;              // Toggle displacement mapping (for terrain)
    int color_mode = WHITE_WIREFRAME;   // Switch color mode of the render
    bool render_projection = true;      // Toggle the MVP matrix
    int interpolation = NONE;           // Switch interpolation mode
    float alpha = 0;                    // Scale interpolation
    int pxEdgeLength = 256;             // Deprecated
    int grid_quads_count = 1;           // Number of quads in the terrain grid 

    // Quadtree Stuff
    bool triangle_mode = true;   // Deprecated
    int prim_type = TRIANGLES;   // Type of primitive of the mesh (changes number of root triangle)
    bool morph = false;          // Toggles T-Junction Removal
    int morph_mode = HYBRID_PRE; // Switch neighbour LoD evaluation mode
    bool freeze = false;         // Toggle freeze i.e. stop updating the quadtree, but keep rendering
    int cpu_lod = 0;             // Control CPU LoD, i.e. subdivision level of the instantiated triangle grid
    bool cull = true;            // Toggles Cull
    bool debug_morph = false;
    float morph_k = 0.0;


    bool modified = false;       // Deprecated

    void UploadSettings(uint pid)
    {
        utility::SetUniformBool(pid, "uniform_subdiv", uniform);
        utility::SetUniformInt(pid, "uniform_level", uni_lvl);
        utility::SetUniformInt(pid, "adaptive_factor", adaptive_factor);
        utility::SetUniformBool(pid, "heightmap", displace);
        utility::SetUniformInt(pid, "color_mode", color_mode);
        utility::SetUniformBool(pid, "render_MVP", render_projection);
        utility::SetUniformInt(pid, "interpolation", interpolation);
        utility::SetUniformFloat(pid, "alpha", alpha);
        utility::SetUniformFloat(pid, "edgePixelLengthTarget", float(pxEdgeLength));
    }

    void UploadQuadtreeSettings(uint pid)
    {
        utility::SetUniformFloat(pid, "cpu_lod", float(cpu_lod));
        utility::SetUniformBool(pid, "morph", morph);
        utility::SetUniformInt(pid, "morph_mode", morph_mode);
        utility::SetUniformBool(pid, "cull", cull);
        utility::SetUniformFloat(pid, "cpu_lod", float(cpu_lod));
        utility::SetUniformInt(pid, "prim_type", prim_type);
        utility::SetUniformBool(pid, "debug_morph", debug_morph);
        utility::SetUniformFloat(pid, "morph_k", morph_k);


    }


    void flagModified (bool b) {
        modified = b;
    }
};

// Nice little struct to manage transforms
struct Transforms
{
    mat4 M     = mat4(1.0);
    mat4 V     = mat4(1.0);
    mat4 P     = mat4(1.0);
    mat4 MVP   = mat4(1.0);
    mat4 MV    = mat4(1.0);
    mat4 invMV = mat4(1.0);
    float fov;
    vec3 cam_pos;

    bool modified = false;

    void UploadTransforms(uint pid)
    {
        utility::SetUniformMat4(pid, "M", M);
        utility::SetUniformMat4(pid, "V", V);
        utility::SetUniformMat4(pid, "P", P);
        utility::SetUniformMat4(pid, "MVP", MVP);
        utility::SetUniformMat4(pid, "MV", MV);
        utility::SetUniformMat4(pid, "invMV", invMV);
        utility::SetUniformVec3(pid, "cam_pos", cam_pos);
    }

    void updateMV()
    {
        MV = V * M;
        MVP = P * MV;
        invMV = glm::transpose(glm::inverse(MV));
        modified = true;
    }

    void flagModified (bool b) {
        modified = b;
    }
};

#endif // COMMON_H
