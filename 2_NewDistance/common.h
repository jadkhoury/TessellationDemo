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

#define GLM_ENABLE_EXPERIMENTAL
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

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

const char* shader_dir = "../2_NewDistance/shaders/";

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec3;
using glm::uvec4;
using glm::mat3;
using glm::mat4;
using glm::uint;
using glm::uint64;

using std::vector;
using std::cout;
using std::endl;
using std::string;
using std::ifstream;
using std::istreambuf_iterator;
using std::runtime_error;
using std::map;


//using namespace std;

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

    void UploadTransforms(uint pid)
    {
        utility::SetUniformMat4(pid, "M", M);
        utility::SetUniformMat4(pid, "V", V);
        utility::SetUniformMat4(pid, "P", P);
        utility::SetUniformMat4(pid, "MVP", MVP);
        utility::SetUniformMat4(pid, "MV", MV);
        utility::SetUniformMat4(pid, "invMV", invMV);
        utility::SetUniformVec3(pid, "cam_pos", cam_pos);
        utility::SetUniformFloat(pid, "fov", fov);
    }

    void updateMV()
    {
        MV = V * M;
        MVP = P * MV;
        invMV = glm::transpose(glm::inverse(MV));
    }
};

#endif // COMMON_H
