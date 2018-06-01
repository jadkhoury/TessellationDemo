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

const char* shader_dir = "../0_FullProgram/src/shaders/";

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

enum { QUADTREE,
       TESS_SHADER,
       PIPELINES_COUNT
     } PipelineModes;

enum {  NEIGHBOUR,
        HYBRID_PRE,
        HYBRID_POST,
        MORPH_MODES_COUNT
     } MorphModes;

enum { TRIANGLES,
       QUADS,
       NUM_TYPES
     } PrimTypes;



struct BufferData {
    GLuint bo, count = 0;
    GLsizei size;
};

struct BufferCombo {
    BufferData v, idx;
    GLuint vao;
};

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

struct Mesh_Data
{
    Vertex* v_array;
    uint* q_idx_array;
    uint* t_idx_array;

    BufferData v, q_idx, t_idx;
    int triangle_count = 0, quad_count = 0;
};

struct Settings
{
    int uni_lvl = 0;
    int adaptive_factor = 1;
    bool uniform = true;
    bool map_primcount = true;
    bool rotateMesh = false;
    bool displace = false;
    int color_mode = WHITE_WIREFRAME;
    bool render_projection = true;
    int interpolation = NONE;
    float alpha = 0;
    int pipeline = QUADTREE;
    int pxEdgeLength = 256;
    int grid_quads_count = 1;

    // Quadtree Stuff
    bool triangle_mode = true;
    int prim_type = TRIANGLES;
    bool morph = false;
    int morph_mode = HYBRID_PRE;
    bool freeze = false;
    int cpu_lod = 0;
    bool cull = true;

    bool modified = false;

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
    }


    void flagModified (bool b) {
        modified = b;
    }
};

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
