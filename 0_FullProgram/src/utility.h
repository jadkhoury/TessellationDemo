#ifndef UTILITY_H
#define UTILITY_H

#include "glm/gtc/type_ptr.hpp"
#include <sstream>
#include <string>

#include "glad/glad.h"

#define LOG(fmt, ...)  fprintf(stdout, fmt, ##__VA_ARGS__); fflush(stdout);


////////////////////////////////////////////////////////////////////////////////
/// OPENGL DEBUG STUFF
///

using std::string;


char *strcat2(char *dst, const char *src1, const char *src2)
{
    strcpy(dst, src1);

    return strcat(dst, src2);
}

////////////////////////////////////////////////////////////////////////////////
/// Struct to facilitate the definition and maintenance of Buffers within OpenGL
///
typedef struct Buffer {
    GLuint bo; //Buffer object
    GLsizei size; //Size in bytes, typically count * sizeof(type)
    GLuint count; //number of elements in the buffer

    void Delete()
    {
        if(glIsBuffer(bo))  glDeleteBuffers(1, &bo);
        bo = 0;
        size = 0;
        count = 0;
    }

    void Reset()
    {
        Delete();
        glCreateBuffers(1, &bo);
    }

} Buffer;

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
/// Small Utility namespace
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


void EmptyBuffer(GLuint* bo)
{
    if(glIsBuffer(*bo))
        glDeleteBuffers(1, bo);
    *bo = 0;
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
}
#endif
