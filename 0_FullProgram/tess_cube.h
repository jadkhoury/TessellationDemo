#ifndef TESSCUBE_H
#define TESSCUBE_H

#include "mesh_utils.h"
#include "common.h"

using namespace std;

class TessCube
{
private:
    Mesh_Data* mesh_data_;
    Settings* set_;
    Transforms* transfo_;
    GLuint vao_;
    GLuint program_;
    bool first_frame_;
    GLuint counter_bo_, zero_bo_, copy_bo_;
    uint prim_count_;
    djg_clock* clock_;


    void configureShaders()
    {
        UploadMeshSettings();
        UploadTransforms();

    }

    bool loadProgram()
    {
        cout << "TessCube - Loading Program... ";

        if(first_frame_)
            program_ = 0;

        djg_program *djp = djgp_create();

        char buf[1024];

        switch (set_->interpolation)
        {
        case NONE:
+            djgp_push_file(djp, strcat2(buf, shader_dir,"LoD.glsl"));
            djgp_push_file(djp, strcat2(buf, shader_dir,"tess_cube.glsl")); break;
        case PN:
//            djgp_push_file(djp, "LoD.glsl");
            djgp_push_file(djp, strcat2(buf, shader_dir,"tess_cube_pn.glsl")); break;
        case PHONG :
//            djgp_push_file(djp, "LoD.glsl");
            djgp_push_file(djp, strcat2(buf, shader_dir,"tess_cube_phong.glsl")); break;
        }

        if (!djgp_to_gl(djp, 450, false, true, &program_))
        {
            djgp_release(djp);
            cout << "X" << endl;
            return false;
        }
        djgp_release(djp);
        configureShaders();
        cout << "OK" << endl;
        return (glGetError() == GL_NO_ERROR);
    }

    void loadVao(){
        glCreateVertexArrays(1, &vao_);
        glVertexArrayAttribBinding(vao_, 0, 0);
        glVertexArrayAttribFormat(vao_, 0, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, p));
        glEnableVertexArrayAttrib(vao_, 0);


        glVertexArrayAttribBinding(vao_, 1, 0);
        glVertexArrayAttribFormat(vao_, 1, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, n));
        glEnableVertexArrayAttrib(vao_, 1);

        glVertexArrayAttribBinding(vao_, 2, 0);
        glVertexArrayAttribFormat(vao_, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
        glEnableVertexArrayAttrib(vao_, 2);

        glVertexArrayVertexBuffer(vao_, 0, mesh_data_->v.bo, 0, sizeof(Vertex));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_data_->t_idx.bo);
    }

    bool loadCounterBo()
    {
        uint zero = 0;

        utility::EmptyBuffer(&counter_bo_);
        glCreateBuffers(1, &counter_bo_);
        glNamedBufferStorage(counter_bo_, sizeof(uint), &zero, 0);


        utility::EmptyBuffer(&zero_bo_);
        glCreateBuffers(1, &zero_bo_);
        glNamedBufferStorage(zero_bo_, sizeof(uint), &zero, 0);


        utility::EmptyBuffer(&copy_bo_);
        glCreateBuffers(1, &copy_bo_);
        glNamedBufferStorage(copy_bo_, sizeof(uint), &zero,
                             GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);


        return (glGetError() == GL_NO_ERROR);
    }

    void copyPrimCount()
    {
        glCopyNamedBufferSubData(counter_bo_, copy_bo_, 0, 0, sizeof(uint));
        uint* data = (uint*) glMapNamedBuffer(copy_bo_, GL_READ_ONLY);
        prim_count_ = data[0];
        glUnmapNamedBuffer(copy_bo_);
    }

public:
    struct Ticks {
        double cpu, gpu;
    } ticks;

    void Init(Mesh_Data* data, Transforms* tr, Settings* settings)
    {
        cout << "******************************************************" << endl;
        cout << "TESSCUBE" << endl;

        first_frame_ = true;
        mesh_data_ = data;
        set_ = settings;
        prim_count_ = 0;
        clock_ = djgc_create();

        transfo_ = tr;
        loadProgram();
        loadVao();
        loadCounterBo();
        transfo_->UploadTransforms(program_);
        glClearColor(1, 1, 1, 1);
        glLineWidth(1.0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glPatchParameteri(GL_PATCH_VERTICES, 3);
    }

    void Draw()
    {
        glEnable(GL_DEPTH_TEST);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        djgc_start(clock_);

        glUseProgram(program_);
        {
            glBindVertexArray(vao_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_data_->t_idx.bo);
            glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_bo_);
            glDrawElements(GL_PATCHES, mesh_data_->t_idx.count, GL_UNSIGNED_INT, NULL);
            glBindVertexArray(0);
        } glUseProgram(0);
        djgc_stop(clock_);
        djgc_ticks(clock_, &ticks.cpu, &ticks.gpu);



        if(set_->map_primcount)
            copyPrimCount();
        glCopyNamedBufferSubData(zero_bo_, counter_bo_, 0, 0, sizeof(uint));

    }

    void ReconfigureShaders() {
        configureShaders();
    }

    void ReloadShaders() {
        loadProgram();
    }

    void UploadMeshSettings() {
        set_->UploadSettings(program_);
    }

    void UploadTransforms() {
        transfo_->UploadTransforms(program_);
    }

    int GetPrimcount() {
        return prim_count_;
    }


};
#endif
