#ifndef QUADTREE_H
#define QUADTREE_H

#include "commands.h"
//#include "mesh_utils.h"
#include "common.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))


class QuadTree
{
    //************************************************************************//
    //                             Private Members                            //
    //************************************************************************//
private:

    Commands* commands_;
    Transforms* transfo_;
    Settings* set_;

    struct ssbo_indices {
        int read = 0;
        int write_full = 1;
        int write_culled = 2;
    } ssbo_idx_;

    const mat4 IDENTITY = mat4(1.0);

    // Buffers and Arrays
    GLuint nodes_bo_[3];

    BufferCombo leaf_geometry;

    // Mesh data
    Mesh_Data* mesh_data_;

    //Programs
    GLuint render_program_, compute_program_, copy_program_;

    //Compute Shader parameters
    const uvec3 local_WG_size_ = vec3(1024,1,1);
    uint local_WG_count = local_WG_size_.x * local_WG_size_.y * local_WG_size_.z;
    uint prim_count_, init_node_count_, init_wg_count_;


    djg_clock* compute_clock;
    djg_clock* render_clock;

    bool first_frame_ = true;

    // --------------------------- SHADER FUNCTIONS ------------------------- //


    void configureComputeProgram()
    {
        utility::SetUniformInt(compute_program_, "num_mesh_tri", mesh_data_->triangle_count);
        utility::SetUniformInt(compute_program_, "num_mesh_quad", mesh_data_->quad_count);
        set_->UploadQuadtreeSettings(compute_program_);
        set_->UploadSettings(compute_program_);
    }

    void configureCopyProgram ()
    {
        utility::SetUniformInt(copy_program_, "num_vertices", leaf_geometry.v.count);
        utility::SetUniformInt(copy_program_, "num_indices", leaf_geometry.idx.count);

    }

    void configureRenderProgram()
    {
        utility::SetUniformInt(render_program_, "num_vertices", leaf_geometry.v.count);
        utility::SetUniformInt(render_program_, "num_indices", leaf_geometry.idx.count);
        set_->UploadQuadtreeSettings(compute_program_);
        set_->UploadSettings(render_program_);
    }

    void pushMacrosToProgram(djg_program* djp)
    {
        djgp_push_string(djp, "#define TRIANGLES %i\n", TRIANGLES);
        djgp_push_string(djp, "#define QUADS %i\n", QUADS);
        djgp_push_string(djp, "#define NODES_IN_B %i\n", NODES_IN_B);
        djgp_push_string(djp, "#define NODES_OUT_FULL_B %i\n", NODES_OUT_FULL_B);
        djgp_push_string(djp, "#define NODES_OUT_CULLED_B %i\n", NODES_OUT_CULLED_B);
        djgp_push_string(djp, "#define DISPATCH_COUNTER_B %i\n", DISPATCH_INDIRECT_B);
        djgp_push_string(djp, "#define DRAW_INDIRECT_B %i\n", DRAW_INDIRECT_B);
        djgp_push_string(djp, "#define NODECOUNTER_FULL_B %i\n", NODECOUNTER_FULL_B);
        djgp_push_string(djp, "#define NODECOUNTER_CULLED_B %i\n", NODECOUNTER_CULLED_B);
        djgp_push_string(djp, "#define LEAF_VERT_B %i\n", LEAF_VERT_B);
        djgp_push_string(djp, "#define LEAF_IDX_B %i\n", LEAF_IDX_B);

        djgp_push_string(djp, "#define MESH_V_B %i\n", MESH_V_B);
        djgp_push_string(djp, "#define MESH_Q_IDX_B %i\n", MESH_Q_IDX_B);
        djgp_push_string(djp, "#define MESH_T_IDX_B %i\n", MESH_T_IDX_B);
        djgp_push_string(djp, "#define LOCAL_WG_SIZE_X %u\n", local_WG_size_.x);
        djgp_push_string(djp, "#define LOCAL_WG_SIZE_Y %u\n", local_WG_size_.y);
        djgp_push_string(djp, "#define LOCAL_WG_SIZE_Z %u\n", local_WG_size_.z);
        djgp_push_string(djp, "#define LOCAL_WG_COUNT %u\n", local_WG_count);

#ifdef ELEMENTS_INDIRECT
        djgp_push_string(djp, "#define ELEMENTS %u\n", 1);
#endif
    }

    bool loadComputeProgram()
    {
        cout << "Quadtree - Loading Compute Program... ";
        if (!glIsProgram(compute_program_))
            compute_program_ = 0;
        djg_program* djp = djgp_create();
        pushMacrosToProgram(djp);
        djgp_push_string(djp, "#define NEIGHBOUR %u\n", NEIGHBOUR);
        djgp_push_string(djp, "#define HYBRID_PRE %u\n", HYBRID_PRE);
        djgp_push_string(djp, "#define HYBRID_POST %u\n", HYBRID_POST);

        char buf[1024];

        djgp_push_file(djp, strcat2(buf, shader_dir, "gpu_noise_lib.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "dj_frustum.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "dj_heightmap.glsl"));

        djgp_push_file(djp, strcat2(buf, shader_dir, "ltree_jk.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "LoD.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "quadtree_compute.glsl"));
        if (!djgp_to_gl(djp, 450, false, true, &compute_program_))
        {
            cout << "X" << endl;
            djgp_release(djp);

            return false;
        }
        djgp_release(djp);
        cout << "OK" << endl;
        configureComputeProgram();
        return (glGetError() == GL_NO_ERROR);
    }

    bool loadCopyProgram()
    {
        cout << "Quadtree - Loading Copy Program... ";
        if (!glIsProgram(copy_program_))
            copy_program_ = 0;
        djg_program* djp = djgp_create();
        pushMacrosToProgram(djp);

        char buf[1024];

        djgp_push_file(djp, strcat2(buf, shader_dir, "quadtree_copy.glsl"));
        if (!djgp_to_gl(djp, 450, false, true, &copy_program_))
        {
            cout << "X" << endl;
            djgp_release(djp);

            return false;
        }
        djgp_release(djp);
        cout << "OK" << endl;
        configureCopyProgram ();
        return (glGetError() == GL_NO_ERROR);
    }


    bool loadRenderProgram()
    {
        cout << "Quadtree - Loading Render Program... ";

        if (!glIsProgram(render_program_))
            render_program_ = 0;
        djg_program *djp = djgp_create();
        pushMacrosToProgram(djp);

        djgp_push_string(djp, "#define WHITE_WIREFRAME %i\n", WHITE_WIREFRAME);
        djgp_push_string(djp, "#define PRIMITIVES %i\n", PRIMITIVES);
        djgp_push_string(djp, "#define LOD %i\n", LOD);
        djgp_push_string(djp, "#define FRUSTUM %i\n", FRUSTUM);
        djgp_push_string(djp, "#define CULL %i\n", CULL);
        djgp_push_string(djp, "#define DEBUG %i\n", DEBUG);
        djgp_push_string(djp, "#define NONE %i\n", NONE);
        djgp_push_string(djp, "#define PN %i\n", PN);
        djgp_push_string(djp, "#define PHONG %i\n", PHONG);

        char buf[1024];

        djgp_push_file(djp, strcat2(buf, shader_dir, "gpu_noise_lib.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "dj_frustum.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "ltree_jk.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "PN_interpolation.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "phong_interpolation.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "LoD.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "dj_heightmap.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "quadtree_render.glsl"));
        if (!djgp_to_gl(djp, 450, false, true, &render_program_))
        {
            cout << "X" << endl;
            djgp_release(djp);
            return false;
        }
        djgp_release(djp);
        cout << "OK" << endl;
        configureRenderProgram();
        return (glGetError() == GL_NO_ERROR);
    }

    bool loadPrograms()
    {
        bool v = true;
        v &= loadComputeProgram();
        v &= loadCopyProgram();
        v &= loadRenderProgram();
        UploadTransforms();
        return v;
    }

    // -------------------------- BUFFER FUNCTIONS --------------------------- //

    bool loadNodesBuffers()
    {
        int max_ssbo_size;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_ssbo_size);
        max_ssbo_size /= 8;
        int max_num_nodes = max_ssbo_size / sizeof(uvec4);
        // cout << "max_num_nodes  " << max_num_nodes << endl;
        // cout << "max_ssbo_size " << max_ssbo_size << "B" << endl;

        uvec4* nodes_array =  new uvec4[max_num_nodes];
        if (set_->prim_type == TRIANGLES) {
            init_node_count_ = 3 * mesh_data_->triangle_count;
            for (int ctr = 0; ctr < mesh_data_->triangle_count; ++ctr) {
                nodes_array[3*ctr+0] = uvec4(0, 0x1, uint(ctr*3), 0);
                nodes_array[3*ctr+1] = uvec4(0, 0x1, uint(ctr*3), 1);
                nodes_array[3*ctr+2] = uvec4(0, 0x1, uint(ctr*3), 2);
            }
        } else if (set_->prim_type == QUADS) {
            init_node_count_ = 4 * mesh_data_->quad_count;
            for (int ctr = 0; ctr < mesh_data_->quad_count; ++ctr) {
                nodes_array[4*ctr+0] = uvec4(0, 0x1, uint(ctr*4), 0);
                nodes_array[4*ctr+1] = uvec4(0, 0x1, uint(ctr*4), 1);
                nodes_array[4*ctr+2] = uvec4(0, 0x1, uint(ctr*4), 2);
                nodes_array[4*ctr+3] = uvec4(0, 0x1, uint(ctr*4), 3);
            }
        }
        init_wg_count_ = ceil(init_node_count_ / float(local_WG_count));
        // cout << "init_node_count_ " << utility::LongToString (init_node_count_) << endl;
        // cout << "init_wg_count_ " << init_wg_count_ << endl;

        utility::EmptyBuffer(&nodes_bo_[0]);
        utility::EmptyBuffer(&nodes_bo_[1]);
        utility::EmptyBuffer(&nodes_bo_[2]);

        glCreateBuffers(3, nodes_bo_);

        glNamedBufferStorage(nodes_bo_[0], max_ssbo_size, nodes_array, 0);
        glNamedBufferStorage(nodes_bo_[1], max_ssbo_size, nodes_array, 0);
        glNamedBufferStorage(nodes_bo_[2], max_ssbo_size, nodes_array, 0);

        return (glGetError() == GL_NO_ERROR);
    }

    vector<vec2> getLeafVertices(uint level)
    {
        vector<vec2> vertices;
        float num_row = 1 << level;
        float col = 0.0, row = 0.0;
        float d = 1.0 / float (num_row);
        while (row <= num_row)
        {
            while (col <= row)
            {
                vertices.push_back(vec2(col * d, 1.0 - row * d));
                col ++;
            }
            row ++;
            col = 0;
        }
        return vertices;
    }

    vector<uvec3> getLeafIndices(uint level)
    {
        vector<uvec3> indices;
        uint col = 0, row = 0;
        uint elem = 0, num_col = 1;
        uint orientation;
        uint num_row = 1 << level;
        auto new_triangle = [&]() {
            if (orientation == 0)
                return uvec3(elem, elem + num_col, elem + num_col + 1);
            else if (orientation == 1)
                return uvec3(elem, elem - 1, elem + num_col);
            else if (orientation == 2)
                return uvec3(elem, elem + num_col, elem + 1);
            else if (orientation == 3)
                return uvec3(elem, elem + num_col - 1, elem + num_col);
            else
                throw std::runtime_error("Bad orientation error");
        };
        while (row < num_row)
        {
            orientation = (row % 2 == 0) ? 0 : 2;
            while (col < num_col)
            {
                indices.push_back(new_triangle());
                orientation = (orientation + 1) % 4;
                if (col > 0) {
                    indices.push_back(new_triangle());
                    orientation = (orientation + 1) % 4;
                }
                col++;
                elem++;
            }
            col = 0;
            num_col++;
            row++;
        }
        return indices;
    }

    bool loadLeafBuffers(uint level)
    {
        vector<vec2> vertices = getLeafVertices(level);
        vector<uvec3> indices = getLeafIndices(level);


        leaf_geometry.v.count = vertices.size();
        leaf_geometry.v.size = leaf_geometry.v.count * sizeof(vec2);
        utility::EmptyBuffer(&leaf_geometry.v.bo);
        glCreateBuffers(1, &leaf_geometry.v.bo);
        glNamedBufferStorage(leaf_geometry.v.bo, leaf_geometry.v.size, (const void*)vertices.data(), 0);

        leaf_geometry.idx.count = indices.size() * 3;
        leaf_geometry.idx.size =leaf_geometry.idx.count * sizeof(uint);
        utility::EmptyBuffer(&leaf_geometry.idx.bo);
        glCreateBuffers(1, &leaf_geometry.idx.bo);
        glNamedBufferStorage(leaf_geometry.idx.bo, leaf_geometry.idx.size, (const void*)indices.data(),  0);
        return (glGetError() == GL_NO_ERROR);
    }

    // --------------------------- VAO FUNCTIONS ----------------------------- //
    bool loadLeafVao()
    {
        if (glIsVertexArray(leaf_geometry.vao)) {
            glDeleteVertexArrays(1, &leaf_geometry.vao);
            leaf_geometry.vao = 0;
        }
        glCreateVertexArrays(1, &leaf_geometry.vao);
#ifdef ELEMENTS_INDIRECT
        glVertexArrayAttribBinding(leaf_geometry.vao, 1, 0);
        glVertexArrayAttribFormat(leaf_geometry.vao, 1, 2, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(leaf_geometry.vao, 1);
        glVertexArrayVertexBuffer(leaf_geometry.vao, 0, leaf_geometry.v.bo, 0, sizeof(vec2));
        glVertexArrayElementBuffer(leaf_geometry.vao, leaf_geometry.idx.bo);
#endif

        return (glGetError() == GL_NO_ERROR);
    }

    // ------------------------- PINGPONG FUNCTIONS --------------------------- //
    void pingpong()
    {
        ssbo_idx_.read = ssbo_idx_.write_full;
        ssbo_idx_.write_full = (ssbo_idx_.read + 1) % 3;
        ssbo_idx_.write_culled = (ssbo_idx_.read + 2) % 3;
    }

    //************************************************************************//
    //                             Public Members                             //
    //************************************************************************//
public:
    struct Ticks {
        double cpu;
        double gpu_compute, gpu_render;
    } ticks;

    // -------------------------- UPDATE FUNCTIONS --------------------------- //

    void ReloadShaders()
    {
        bool v = loadPrograms();
    }

    void ReconfigureShaders()
    {
        configureComputeProgram();
        configureRenderProgram();
    }

    void Reinitialize()
    {
        loadLeafBuffers(set_->cpu_lod);
        loadLeafVao();
        loadNodesBuffers();
        loadPrograms();
        commands_->ReinitializeCommands(leaf_geometry.idx.count, init_wg_count_, init_node_count_);
    }

    int GetPrimcount()
    {
        return prim_count_;
    }

    void ReloadLeafPrimitive()
    {
        loadLeafBuffers(set_->cpu_lod);
        loadLeafVao();
        configureComputeProgram ();
        configureCopyProgram ();
        configureRenderProgram ();
        commands_->UpdateLeafGeometry(leaf_geometry.idx.count);
    }

    void UploadTransforms()
    {
        transfo_->UploadTransforms(compute_program_);
        transfo_->UploadTransforms(render_program_);
    }

    void UploadMeshSettings()
    {
        set_->UploadSettings(compute_program_);
        set_->UploadSettings(render_program_);
    }

    void UploadQuadtreeSettings()
    {
        set_->UploadQuadtreeSettings(compute_program_);
        set_->UploadQuadtreeSettings(render_program_);
    }

    // ----------------------------- ZEE PROGRAM ----------------------------- //

    void Init(Mesh_Data* m_data, Transforms* transfo, Settings* settings)
    {
        cout << "******************************************************" << endl;
        cout << "QUADTREE" << endl;
        mesh_data_ = m_data;
        transfo_ = transfo;
        set_ = settings;

        prim_count_ = 0;
        commands_ = new Commands();
        compute_clock = djgc_create();
        render_clock = djgc_create();


        loadLeafBuffers(set_->cpu_lod);
        loadLeafVao();
        loadNodesBuffers();

        if (!loadPrograms())
            throw std::runtime_error("shader creation error");

        commands_->Init(leaf_geometry.idx.count, init_wg_count_, init_node_count_);

        ReconfigureShaders();

        glUseProgram(0);

    }

    void Draw(float deltaT)
    {
        if (set_->freeze)
            goto RENDER_PASS;

        if (!first_frame_)
            pingpong ();

        // ----- Compute Pass ----- //
        glEnable(GL_RASTERIZER_DISCARD);
        djgc_start(compute_clock);
        glUseProgram(compute_program_);
        {
            utility::SetUniformFloat(compute_program_, "deltaT", deltaT);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODES_IN_B, nodes_bo_[ssbo_idx_.read]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODES_OUT_FULL_B, nodes_bo_[ssbo_idx_.write_full]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODES_OUT_CULLED_B, nodes_bo_[ssbo_idx_.write_culled]);
            commands_->BindForCompute(compute_program_);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_V_B, mesh_data_->v.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_Q_IDX_B, mesh_data_->q_idx.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_T_IDX_B, mesh_data_->t_idx.bo);

            glDispatchComputeIndirect((long)NULL);
            glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
        }
        glUseProgram(0);

        // ----- Copy Pass ----- //
        glUseProgram(copy_program_);
        {
            commands_->BindForCopy(copy_program_);
            glBindBufferBase(GL_UNIFORM_BUFFER, LEAF_VERT_B, leaf_geometry.v.bo);
            glBindBufferBase(GL_UNIFORM_BUFFER, LEAF_IDX_B, leaf_geometry.idx.bo);
            glDispatchCompute(1,1,1);
            glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
        }
        glUseProgram(0);

        djgc_stop(compute_clock);
        djgc_ticks(compute_clock, &ticks.cpu, &ticks.gpu_compute);

        // commands_->PrintWGCountInDispatch();
        // commands_->PrintAtomicArray();

        glDisable(GL_RASTERIZER_DISCARD);

RENDER_PASS:
        if (set_->map_primcount) {
            prim_count_ = commands_->GetPrimCount();
        }

        // ----- Render Pass ----- //
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glClearDepth(1.0);
        glClearColor(1,1,1,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(render_program_);
        {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODES_IN_B, nodes_bo_[ssbo_idx_.write_culled]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_V_B, mesh_data_->v.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_Q_IDX_B, mesh_data_->q_idx.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_T_IDX_B, mesh_data_->t_idx.bo);
            commands_->BindForRender();
            glBindVertexArray(leaf_geometry.vao);
            djgc_start(render_clock);
#ifdef ELEMENTS_INDIRECT
            glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
#else
            glBindBufferBase(GL_UNIFORM_BUFFER, LEAF_VERT_B, leaf_geometry.v.bo);
            glBindBufferBase(GL_UNIFORM_BUFFER, LEAF_IDX_B, leaf_geometry.idx.bo);
            glDrawArraysIndirect(GL_TRIANGLES, BUFFER_OFFSET(0));
#endif
            glBindVertexArray(0);
            djgc_stop(render_clock);
        }
        glUseProgram(0);
        djgc_ticks(render_clock, &ticks.cpu, &ticks.gpu_render);

        if (first_frame_)
            first_frame_ = false;
    }

    void CleanUp()
    {
        glUseProgram(0);
        glDeleteBuffers(3, nodes_bo_);
        glDeleteProgram(compute_program_);
        glDeleteProgram(copy_program_);
        glDeleteProgram(render_program_);
        glDeleteBuffers(1, &leaf_geometry.v.bo);
        glDeleteBuffers(1, &leaf_geometry.idx.bo);
        glDeleteVertexArrays(1, &leaf_geometry.vao);
        commands_->Cleanup();
    }


};

#endif
