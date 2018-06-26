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
struct uniforms_block
{
    mat4 M     = mat4(1.0);
    mat4 V     = mat4(1.0);
    mat4 P     = mat4(1.0);
    mat4 MVP   = mat4(1.0);
    mat4 MV    = mat4(1.0);
    mat4 invMV = mat4(1.0);
    vec4 frustum_planes[6];

    vec3 cam_pos = vec3(1.0);
    float fov = 45.0;
};

struct TransformsManager
{

    GLuint bo;
    uniforms_block transforms;
    bool modified;

    void Init()
    {
        glCreateBuffers(1, &bo);
        glNamedBufferData(bo, sizeof(uniforms_block), NULL, GL_STREAM_DRAW);
    }

    void updateFrustum()
    {
        mat4& MVP = transforms.MVP;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 2; ++j) {
                transforms.frustum_planes[i*2+j].x = MVP[0][3] + (j == 0 ? MVP[0][i] : -MVP[0][i]);
                transforms.frustum_planes[i*2+j].y = MVP[1][3] + (j == 0 ? MVP[1][i] : -MVP[1][i]);
                transforms.frustum_planes[i*2+j].z = MVP[2][3] + (j == 0 ? MVP[2][i] : -MVP[2][i]);
                transforms.frustum_planes[i*2+j].w = MVP[3][3] + (j == 0 ? MVP[3][i] : -MVP[3][i]);
                vec3 tmp = vec3(transforms.frustum_planes[i*2+j]);
                transforms.frustum_planes[i*2+j] *= glm::length(tmp);
            }
    }

    void UploadTransforms()
    {
        if(modified) {
            modified = false;
            glNamedBufferData(bo, sizeof(uniforms_block), &transforms, GL_STREAM_DRAW);
            cout << "uploading" << endl;
        }
    }

    void UpdateMV()
    {
        transforms.MV = transforms.V * transforms.M;
        transforms.MVP = transforms.P * transforms.MV;
        transforms.invMV = glm::transpose(glm::inverse(transforms.MV));
        updateFrustum();
        modified = true;

    }
};

#endif // COMMON_H
