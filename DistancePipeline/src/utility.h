#ifndef UTILITY_H
#define UTILITY_H

#include "glm/gtc/type_ptr.hpp"
#include <sstream>
#include <string>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "tiny_obj_loader.h"

using std::string;



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

void EmptyBuffer(GLuint* buf)
{
    if(glIsBuffer(*buf))
        glDeleteBuffers(1, buf);
    *buf = 0;
}

/*
static void loadTextureFromFile(string filename, GLuint* id, bool mipmap){
    // load texture
    int width;
    int height;
    int nb_component;
    // set stb_image to have the same coordinates as OpenGL
    stbi_set_flip_vertically_on_load(1);
    unsigned char* image = stbi_load(filename.c_str(), &width,
                                     &height, &nb_component, 0);
    if(image == nullptr) {
        throw(string("Failed to load texture"));
    }
    glGenTextures(1, id);
    glBindTexture(GL_TEXTURE_2D, *id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if(mipmap > 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0.0);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 3.0);
    } else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if(nb_component == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, image);
    } else if(nb_component == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, image);
    }
    if(mipmap>0)
        glGenerateMipmap(GL_TEXTURE_2D);
    // cleanup
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(image);
}
*/
}
#endif
