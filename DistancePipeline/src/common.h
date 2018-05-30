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

////////////////////////////////////////////////////////////////////////////////
///
/// Rendering Utilities
///

enum {LOD,
      WHITE,
      PRIMITIVES,
      NUM_COLOR_MODES} ColorModes;

enum { TRIANGLES,
       QUADS,
       NUM_TYPES };

struct Vertex {
    vec4 p;
    vec4 n;
    vec2 uv;
    vec2 align;
};

struct BufferData {
    GLuint bo, count;
    GLsizei size;
};

struct BufferCombo {
    BufferData v;
    BufferData idx;
    GLuint vao;
};

struct Mesh_Data
{
    Vertex* v_array;
    uint* q_idx_array;
    uint* t_idx_array;

    BufferData v, q_idx, t_idx;
    int triangle_count, quad_count;
};

static char const * sgets( char * s, int size, char ** stream ) {
    for (int i=0; i<size; ++i) {
        if ( (*stream)[i]=='\n' || (*stream)[i]=='\0') {

            memcpy(s, *stream, i);
            s[i]='\0';

            if ((*stream)[i]=='\0')
                return 0;
            else {
                (*stream) += i+1;
                return s;
            }
        }
    }
    return 0;
}

string LongToString(long l)
{
    bool neg = (l<0);
    if (neg) l = -l;
    string s = "";
    int mod;
    while (l > 0) {
        mod = l%1000;
        s = std::to_string(mod) +  s;
        if (l>999) {
            if (mod < 10) s = "0" + s;
            if (mod < 100) s = "0" + s;
            s = " " + s;
        }
        l /= 1000;
    }
    if (neg) s = "-" + s;
    return s;
}

bool ParseObj(string file_path, int axis, Mesh_Data& mesh_data)
{
    // Opening file and stuff
    ifstream instream(file_path);
    string contents((istreambuf_iterator<char>(instream)), istreambuf_iterator<char>());

    if (contents.empty())
        return false;

    char const* shapestr = contents.c_str();

    char* str = const_cast<char *>(shapestr), line[256];
    bool done = false;

    // Filling independent vectors from the data
    vector<vec4> verts;
    vector<vec2> uvs;
    vector<vec4> normals;
    vector<int> faceverts;
    vector<int> faceuvs;
    vector<int> facenormals;
    vec3 p, n, max = vec3(0);
    int nvertsPerFace = -1;
    while (! done) {
        done = sgets(line, sizeof(line), &str)==0;
        char* end = &line[strlen(line)-1];
        if (*end == '\n') *end = '\0'; // strip trailing nl
        float x, y, z, u, v;
        switch (line[0]) {
        case 'v':
            switch (line[1]) {
            case ' ':
                if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3) {
                    p = (axis == 0) ? vec3(x, -z, y) : vec3(x,y,z);
                    max = glm::max(max, glm::abs(p));
                    verts.push_back(vec4(p, 1.0));
                } break;
            case 't':
                if (sscanf(line, "vt %f %f", &u, &v) == 2) {
                    uvs.push_back(vec2(u,v));
                } break;
            case 'n' :
                if (sscanf(line, "vn %f %f %f", &x, &y, &z) == 3) {
                    n = (axis == 0) ? vec3(x, -z, y) : vec3(x,y,z);
                    normals.push_back(vec4(glm::normalize(n), 0.0));
                } break;
            } break;
        case 'f':
            if (line[1] == ' ') {
                int vi = -1, ti = -1, ni = -1;
                const char* cp = &line[2];
                while (*cp == ' ') cp++;
                int nverts = 0, nitems=0;
                while( (nitems=sscanf(cp, "%d/%d/%d", &vi, &ti, &ni))>0) {
                    nverts++;
                    faceverts.push_back(vi-1);
                    if(ti > 0) faceuvs.push_back(ti-1);
                    if(ni > 0) facenormals.push_back(ni-1);
                    while (*cp && *cp != ' ') cp++;
                    while (*cp == ' ') cp++;
                }
                if (nvertsPerFace == -1){
                    nvertsPerFace = nverts;
                }
                if (nverts != 3 && nverts != 4)
                    throw runtime_error("Need quads or triangle faces");
                if (nverts != nvertsPerFace)
                    throw runtime_error("Need faces with same number of vertices");
            } break;
        default:
            break;
        }
    }

    float m = glm::compMax(max);
    for (int i = 0; i < verts.size(); ++i) {
        verts[i] /= vec4(m,m,m,1);
    }

    // Putting this data in well constructed data structure
    vector<Vertex> vert_vector;
    vector<uint> idx_vector;
    std::map<uint, uint> uniqueIDX_to_vectorIDX;
    Vertex current_v;
    int num_idx = faceverts.size();
    int Nv = verts.size();
    int Nn = normals.size();
    int Nt = uvs.size();
    cout << "Mesh Data:" << endl;
    cout << Nv << " Vertice, " << endl;
    cout << Nn << " Normals, " << endl;
    cout << Nt << " UVs" << endl;
    int iv = 0, it = 0, in = 0;
    ulong unique_idx;
    bool with_normals = facenormals.size() > 0;
    bool with_uvs = faceuvs.size() > 0;

    for (int i = 0;  i < num_idx; i++)
    {
        iv = faceverts[i];
        if (with_normals) in = facenormals[i];
        if (with_uvs)  it = faceuvs[i];
        unique_idx = iv + Nv * (in + Nn * it);
        if (uniqueIDX_to_vectorIDX.count(unique_idx) == 0) {
            current_v.p = verts[iv];
            if (with_normals)  current_v.n = normals[in];
            if (with_uvs) current_v.uv = uvs[it];
            uniqueIDX_to_vectorIDX[unique_idx] = vert_vector.size();
            vert_vector.push_back(current_v);
             // cout << "Created vertex " << unique_idx << " at idx " << uniqueIDX_to_vectorIDX[unique_idx] << endl;
             // cout << current_v.to_string() << endl;
        } else {
             // cout << "Vertex at " << unique_idx << " already created at  "<< uniqueIDX_to_vectorIDX[unique_idx]  << endl;
             // cout << vert_vector[uniqueIDX_to_vectorIDX[unique_idx]].to_string() << endl;
        }
        idx_vector.push_back(uniqueIDX_to_vectorIDX[unique_idx]);
    }
    cout << vert_vector.size() << " unique vertices created" << endl;
    cout << faceverts.size() << " faceverts" << endl;
    cout << facenormals.size() << " facenormals" << endl;

    //Putting data in Mesh_data structure
    if (nvertsPerFace == 3) {
        mesh_data.t_idx_array = new uint[idx_vector.size()];
        std::copy(idx_vector.begin(), idx_vector.end(), mesh_data.t_idx_array);
        mesh_data.t_idx.count = idx_vector.size();
        mesh_data.q_idx.count = 0;
    } else {
        mesh_data.q_idx_array = new uint[idx_vector.size()];
        std::copy(idx_vector.begin(), idx_vector.end(), mesh_data.q_idx_array);
        mesh_data.q_idx.count = idx_vector.size();
        mesh_data.t_idx.count = 0;
    }
    mesh_data.v_array = new Vertex[vert_vector.size()];
    std::copy(vert_vector.begin(), vert_vector.end(), mesh_data.v_array);
    mesh_data.v.count = vert_vector.size();
    mesh_data.triangle_count = mesh_data.t_idx.count / 3;
    mesh_data.quad_count = mesh_data.q_idx.count / 4;

    // Deleting old data structures
    verts.clear();
    uvs.clear();
    normals.clear();
    faceverts.clear();
    faceuvs.clear();
    facenormals.clear();
    idx_vector.clear();
    vert_vector.clear();
    return true;
}


////////////////////////////////////////////////////////////////////////////////
///
/// OPENGL DEBUG
///

#define LOG(fmt, ...)  fprintf(stdout, fmt, ##__VA_ARGS__); fflush(stdout);
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

const char* shader_dir = "../src/shaders/";

char *strcat2(char *dst, const char *src1, const char *src2)
{
    strcpy(dst, src1);

    return strcat(dst, src2);
}

static void
debug_output_logger(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const GLvoid* userParam
        ) {
    char srcstr[32], typestr[32];

    switch(source) {
    case GL_DEBUG_SOURCE_API: strcpy(srcstr, "OpenGL"); break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: strcpy(srcstr, "Windows"); break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: strcpy(srcstr, "Shader Compiler"); break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: strcpy(srcstr, "Third Party"); break;
    case GL_DEBUG_SOURCE_APPLICATION: strcpy(srcstr, "Application"); break;
    case GL_DEBUG_SOURCE_OTHER: strcpy(srcstr, "Other"); break;
    default: strcpy(srcstr, "???"); break;
    };

    switch(type) {
    case GL_DEBUG_TYPE_ERROR: strcpy(typestr, "Error"); break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: strcpy(typestr, "Deprecated Behavior"); break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: strcpy(typestr, "Undefined Behavior"); break;
    case GL_DEBUG_TYPE_PORTABILITY: strcpy(typestr, "Portability"); break;
    case GL_DEBUG_TYPE_PERFORMANCE: strcpy(typestr, "Performance"); break;
    case GL_DEBUG_TYPE_OTHER: strcpy(typestr, "Message"); break;
    default: strcpy(typestr, "???"); break;
    }

    if(severity == GL_DEBUG_SEVERITY_HIGH) {
        LOG("djg_error: %s %s\n"                \
            "-- Begin -- GL_debug_output\n" \
            "%s\n"                              \
            "-- End -- GL_debug_output\n",
            srcstr, typestr, message);
    } else if(severity == GL_DEBUG_SEVERITY_MEDIUM) {
        LOG("djg_warn: %s %s\n"                 \
            "-- Begin -- GL_debug_output\n" \
            "%s\n"                              \
            "-- End -- GL_debug_output\n",
            srcstr, typestr, message);
    }
}

void log_debug_output(void)
{
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&debug_output_logger, NULL);
}


////////////////////////////////////////////////////////////////////////////////
///
/// Utility Namespace
///

namespace utility {

static void SetUniformBool(GLuint pid, string name, bool val)
{
    GLuint location = glGetUniformLocation(pid, name.c_str());
    glProgramUniform1i(pid, location, int(val));
}

static void SetUniformInt(GLuint pid, string name, int val)
{
    GLuint location = glGetUniformLocation(pid, name.c_str());
    glProgramUniform1i(pid, location, val);
}

static void SetUniformFloat(GLuint pid, string name, float val)
{
    GLuint location = glGetUniformLocation(pid, name.c_str());
    glProgramUniform1f(pid, location, val);
}

static void SetUniformVec2(GLuint pid, string name, const glm::vec2& value)
{
    GLuint location = glGetUniformLocation(pid, name.c_str());
    glProgramUniform2fv(pid, location, 1, glm::value_ptr(value));
}

static void SetUniformVec3(GLuint pid, string name, const glm::vec3& value)
{
    GLuint location = glGetUniformLocation(pid, name.c_str());
    glProgramUniform3fv(pid, location, 1, glm::value_ptr(value));
}

static void SetUniformMat3(GLuint pid, string name, const glm::mat3& value)
{
    GLuint location = glGetUniformLocation(pid, name.c_str());
    glProgramUniformMatrix3fv(pid, location, 1, GL_FALSE, glm::value_ptr(value));
}

static void SetUniformMat4(GLuint pid, string name, const glm::mat4& value)
{
    GLuint location = glGetUniformLocation(pid, name.c_str());
    glProgramUniformMatrix4fv(pid, location, 1, GL_FALSE, glm::value_ptr(value));
}

string LongToString(long l)
{
    bool neg = (l<0);
    if (neg) l = -l;
    string s = "";
    int mod;
    while (l > 0) {
        mod = l%1000;
        s = std::to_string(mod) +  s;
        if (l>999) {
            if (mod < 10) s = "0" + s;
            if (mod < 100) s = "0" + s;
            s = " " + s;
        }
        l /= 1000;
    }
    if (neg) s = "-" + s;
    return s;
}

void DeleteBuffer(GLuint* buf)
{
    if(glIsBuffer(*buf))
        glDeleteBuffers(1, buf);
    *buf = 0;
}
}
#endif
