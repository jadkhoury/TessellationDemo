#ifndef POINT_H
#define POINT_H

#include "utility.h"
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec4;
using glm::bvec3;
using glm::ivec3;

using namespace std;

class Point
{

private:
    vec2 position_;
    float height_;

    vec3 translation_;
    float factor_;

    GLuint vao_;
    GLuint program_;

    bool first_frame_;

    bool loadProgram()
    {
        cout << "Point - Loading Program... ";
        if(first_frame_)
            program_ = 0;
        djg_program *djp = djgp_create();

        char buf[1024];
        djgp_push_file(djp, strcat2(buf, shader_dir,"point.glsl"));
        if (!djgp_to_gl(djp, 450, false, true, &program_))
        {
            djgp_release(djp);
            cout << "X" << endl;
            return false;
        }
        djgp_release(djp);
        cout << "OK" << endl;
        return (glGetError() == GL_NO_ERROR);
    }

    void updatePos(float deltaT)
    {
        position_ += vec2(translation_) * vec2(factor_ * deltaT);

        height_ += translation_.z * factor_ * deltaT;
        height_ = glm::clamp(height_, 0.0f, 5.0f);

        translation_ = vec3(0.0f);
    }

public:
    void Init()
    {
        cout << "******************************************************" << endl;
        cout << "POINT" << endl;
        first_frame_ = true;
        position_ = vec2(0.0);
        height_ = 0.5;
        translation_ = vec3(0.0);
        factor_ = 1.0;

        glCreateVertexArrays(1, &vao_);
        if(!loadProgram())
            throw std::runtime_error("Shader creation error");

    }

    void Draw(float deltaT)
    {
        if(translation_ != vec3(0.0f))
            updatePos(deltaT);
        glUseProgram(program_);
        {
            glPointSize(4.0);
            glBindVertexArray(vao_);
            utility::SetUniformVec2(program_, "pos2D", position_);
            glDrawArrays(GL_POINTS, 0, 1);
        } glUseProgram(0);
    }

    void Translate(ivec3 dp)
    {
        translation_ = dp;
    }

    vec3 Get3DPos()
    {
        return vec3(position_, height_);
    }

    vec2 Get2DPos()
    {
        return position_;
    }

    void CleanUp()
    {
        glUseProgram(0);
        glDeleteBuffers(1, &vao_);
        glDeleteProgram(program_);
    }

    void ReinitializePosition()
    {
        position_ = vec2(0.0);
        height_ = 0.5;
    }

};

#endif
