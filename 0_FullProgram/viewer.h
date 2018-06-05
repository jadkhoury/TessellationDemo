#ifndef VIEWER_H
#define VIEWER_H

#include "common.h"

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
        cout << "Viewer - Loading Program... ";
        if (first_frame_)
            program_ = 0;
        djg_program *djp = djgp_create();
        char buf[1024];
        djgp_push_file(djp, strcat2(buf, shader_dir,"viewer.glsl"));
        if (!djgp_to_gl(djp, 450, false, true, &program_)) {
            djgp_release(djp);
            cout << "X" << endl;
            return false;
        }
        djgp_release(djp);
        cout << "OK" << endl;
        return (glGetError() == GL_NO_ERROR);
    }


public:
    void Init(GLuint tex, int w, int h)
    {
        cout << "******************************************************" << endl;
        cout << "VIEWER" << endl;
        tex_ = tex;
        tex_w_ = w;
        tex_h_ = h;
        first_frame_ = true;
        glCreateVertexArrays(1, &vao_);
        if (!loadProgram())
            throw std::runtime_error("Shader creation error");
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
