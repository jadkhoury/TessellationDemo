#ifndef VIEWER_H
#define VIEWER_H

#include "utility.h"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec4;

using namespace std;

class Viewer
{
private:
    GLuint program_;
    GLuint vao_;
    GLuint tex_;
    int tex_w_, tex_h_;
    bool first_frame_;

    bool loadProgram()
    {
        cout << "---> Loading Viewer Program" << endl;
        if(first_frame_)
            program_ = 0;
        djg_program *djp = djgp_create();
        djgp_push_file(djp, "viewer.glsl");
        if (!djgp_gl_upload(djp, 450, false, true, &program_))
        {
            djgp_release(djp);
            cout << "-----> Failure" << endl;
            return false;
        }
        djgp_release(djp);
        cout << "-----> Success" << endl;
        return (glGetError() == GL_NO_ERROR);
    }


public:
    void Init(GLuint tex, int w, int h)
    {
        cout << "-> VIEWER" << endl;
        tex_ = tex;
        tex_w_ = w;
        tex_h_ = h;
        first_frame_ = true;
        glCreateVertexArrays(1, &vao_);
        if(!loadProgram())
            throw std::runtime_error("Shader creation error");
        cout << "-> END VIEWER" << endl << endl;
    }

    void Draw()
    {
        glBindTexture(GL_TEXTURE_2D, tex_);
        glUseProgram(program_);
        {
            glBindVertexArray(vao_);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
        }
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);

    }
};


#endif
