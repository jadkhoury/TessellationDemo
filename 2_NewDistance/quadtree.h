#ifndef QUADTREE_H
#define QUADTREE_H

#include "commands.h"
#include "common.h"

class QuadTree
{

public:
    struct Settings
    {
        bool uniform_on;           // Toggle uniform subdivision
        int uniform_lvl;            // Level of uniform subdivision
        float adaptive_factor;  // Factor scaling the adaptive subdivision
        bool map_primcount;     // Toggle the readback of the node counters
        bool rotateMesh;        // Toggle mesh rotation (for mesh)
        bool displace;          // Toggle displacement mapping (for terrain)
        int color_mode;         // Switch color mode of the render
        bool projection_on; // Toggle the MVP matrix

        int poly_type;    // Type of polygon of the mesh (changes number of root triangle)
        bool morph_on;    // Toggle T-Junction Removal
        bool freeze;      // Toggle freeze i.e. stop updating the quadtree, but keep rendering
        int cpu_lod;      // Control CPU LoD, i.e. subdivision level of the instantiated triangle grid
        bool cull_on;     // Toggle Cull
        bool morph_debug; // Toggle morph debuging
        float morph_k;    // Control morph factor

        bool ipl_on;
        float ipl_alpha;


        uint wg_count; // Control morph factor

        void Upload(uint pid)
        {
            utility::SetUniformBool(pid, "uniform_subdiv", uniform_on);
            utility::SetUniformInt(pid, "uniform_level", uniform_lvl);
            utility::SetUniformFloat(pid, "adaptive_factor", adaptive_factor);
            utility::SetUniformBool(pid, "heightmap", displace);
            utility::SetUniformInt(pid, "color_mode", color_mode);
            utility::SetUniformBool(pid, "render_MVP", projection_on);
            utility::SetUniformBool(pid, "cull", cull_on);
            utility::SetUniformFloat(pid, "cpu_lod", float(cpu_lod));
            utility::SetUniformInt(pid, "poly_type", poly_type);
            utility::SetUniformBool(pid, "morph", morph_on);
            utility::SetUniformBool(pid, "morph_debug", morph_debug);
            utility::SetUniformFloat(pid, "morph_k", morph_k);

            utility::SetUniformBool(pid, "ipl_on", ipl_on);
            utility::SetUniformFloat(pid, "ipl_alpha", ipl_alpha);
        }
    } settings;
    uint full_node_count_, drawn_node_count;


private:
    Commands* commands_;
    TransformsManager* transfo_;

    struct ssbo_indices {
        int read = 0;
        int write_full = 1;
        int write_culled = 2;
    } ssbo_idx_;

    // Buffers and Arrays
    GLuint nodes_bo_[3];

    BufferCombo leaf_geometry;

    // Mesh data
    Mesh_Data* mesh_data_;

    //Programs
    GLuint render_program_, compute_program_, copy_program_;

    //Compute Shader parameters
    uvec3 local_WG_size_;
    uint local_WG_count;
    uint init_node_count_, init_wg_count_;
    int max_node_count_;

    djg_clock* compute_clock;
    djg_clock* render_clock;

    bool first_frame_ = true;


    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// Shader functions
    ///

    void configureComputeProgram()
    {
        utility::SetUniformInt(compute_program_, "num_mesh_tri", mesh_data_->triangle_count);
        utility::SetUniformInt(compute_program_, "num_mesh_quad", mesh_data_->quad_count);
        utility::SetUniformInt(compute_program_, "max_node_count", max_node_count_);

        settings.Upload(compute_program_);
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
        settings.Upload(render_program_);
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
    }

    bool loadComputeProgram()
    {
        cout << "Quadtree - Loading Compute Program... ";
        if (!glIsProgram(compute_program_))
            compute_program_ = 0;
        djg_program* djp = djgp_create();
//        djgp_push_string(djp, "#extension ARB_shader_atomic_counter_ops : require \n");
        pushMacrosToProgram(djp);
        char buf[1024];
        djgp_push_file(djp, strcat2(buf, shader_dir, "gpu_noise_lib.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "noise.glsl"));


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

        char buf[1024];

        djgp_push_file(djp, strcat2(buf, shader_dir, "gpu_noise_lib.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "ltree_jk.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "LoD.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "noise.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "PN_interpolation.glsl"));


        djgp_push_file(djp, strcat2(buf, shader_dir, "quadtree_render_flat.glsl"));
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
        return v;
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// Buffer Function
    ///

    bool loadNodesBuffers()
    {
        int max_ssbo_size;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_ssbo_size);
        max_ssbo_size /= 8;
        max_node_count_ = max_ssbo_size / sizeof(uvec4);
         cout << "max_num_nodes  " << max_node_count_ << endl;
         cout << "max_ssbo_size " << max_ssbo_size << "B" << endl;
        uvec4* nodes_array =  new uvec4[max_node_count_];
        if (settings.poly_type == TRIANGLES) {
            init_node_count_ = mesh_data_->triangle_count;
            for (int ctr = 0; ctr < init_node_count_; ++ctr) {
                nodes_array[ctr] = uvec4(0, 0x1, uint(ctr*3), 0);
            }
        } else if (settings.poly_type == QUADS) {
            init_node_count_ = 2 * mesh_data_->quad_count;
            for (int ctr = 0; ctr < init_node_count_; ++ctr) {
                nodes_array[2*ctr+0] = uvec4(0, 0x1, uint(ctr*4), 0);
                nodes_array[2*ctr+1] = uvec4(0, 0x1, uint(ctr*4), 1);
            }
        }

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

    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// VAO functions
    ///

    bool loadLeafVao()
    {
        if (glIsVertexArray(leaf_geometry.vao)) {
            glDeleteVertexArrays(1, &leaf_geometry.vao);
            leaf_geometry.vao = 0;
        }
        glCreateVertexArrays(1, &leaf_geometry.vao);
        glVertexArrayAttribBinding(leaf_geometry.vao, 1, 0);
        glVertexArrayAttribFormat(leaf_geometry.vao, 1, 2, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(leaf_geometry.vao, 1);
        glVertexArrayVertexBuffer(leaf_geometry.vao, 0, leaf_geometry.v.bo, 0, sizeof(vec2));
        glVertexArrayElementBuffer(leaf_geometry.vao, leaf_geometry.idx.bo);

        return (glGetError() == GL_NO_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// Pingpong functions
    ///
    void pingpong()
    {
        ssbo_idx_.read = ssbo_idx_.write_full;
        ssbo_idx_.write_full = (ssbo_idx_.read + 1) % 3;
        ssbo_idx_.write_culled = (ssbo_idx_.read + 2) % 3;
    }

public:
    struct Ticks {
        double cpu;
        double gpu_compute, gpu_render;
    } ticks;

    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// Update function
    ///

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
        loadLeafBuffers(settings.cpu_lod);
        loadLeafVao();
        loadNodesBuffers();
        loadPrograms();
        commands_->Init(leaf_geometry.idx.count, init_wg_count_, init_node_count_);
    }

    void ReloadLeafPrimitive()
    {
        loadLeafBuffers(settings.cpu_lod);
        loadLeafVao();
        configureComputeProgram ();
        configureCopyProgram ();
        configureRenderProgram ();
        commands_->UpdateLeafGeometry(leaf_geometry.idx.count);
    }

    void UploadSettings()
    {
        settings.Upload(compute_program_);
        settings.Upload(render_program_);
    }

    void UpdateWGCount()
    {
        local_WG_size_ = vec3(settings.wg_count,1,1);
        local_WG_count = local_WG_size_.x * local_WG_size_.y * local_WG_size_.z;
        init_wg_count_ = ceil(init_node_count_ / float(local_WG_count));

        Reinitialize();
    }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// The Program
    ///
    ///
    /*
     * Initialize the Quadtree:
     * - Recieve the mesh data and transform poniters
     * - Sets the settings to their initial values
     * - Generate the leaf geometry
     * - Load the buffers for the nodes and the leaf geometry
     * - Load the glsl programs
     * - Initialize the command class instance
     * - Update the uniform values once again, after all these loadings
     */
    void Init(Mesh_Data* m_data, TransformsManager* transfo, const Settings& init_settings)
    {
        cout << "******************************************************" << endl;
        cout << "QUADTREE" << endl;
        mesh_data_ = m_data;
        transfo_ = transfo;
        settings = init_settings;

        commands_ = new Commands();
        compute_clock = djgc_create();
        render_clock = djgc_create();

        local_WG_size_ = vec3(settings.wg_count,1,1);
        local_WG_count = local_WG_size_.x * local_WG_size_.y * local_WG_size_.z;

        loadLeafBuffers(settings.cpu_lod);
        loadLeafVao();
        loadNodesBuffers();

        init_wg_count_ = ceil(init_node_count_ / float(local_WG_count));

        if (!loadPrograms())
            throw std::runtime_error("shader creation error");

        transfo_->Init();
        commands_->Init(leaf_geometry.idx.count, init_wg_count_, init_node_count_);

        ReconfigureShaders();

        glUseProgram(0);

    }

    /*
     * Render function
     */
    void Draw(float deltaT)
    {
        if (settings.freeze)
            goto RENDER_PASS;

        pingpong ();

        /*
         * COMPUTE PASS
         * - Reads the keys in the SSBO
         * - Evaluates the LoD
         * - Writes the new keys in opposite SSBO
         * - Performs culling
         */
        glEnable(GL_RASTERIZER_DISCARD);
        djgc_start(compute_clock);
        glUseProgram(compute_program_);
        {
            utility::SetUniformFloat(compute_program_, "deltaT", deltaT);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODES_IN_B, nodes_bo_[ssbo_idx_.read]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODES_OUT_FULL_B, nodes_bo_[ssbo_idx_.write_full]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODES_OUT_CULLED_B, nodes_bo_[ssbo_idx_.write_culled]);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, transfo_->bo);
            commands_->BindForCompute(compute_program_);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_V_B, mesh_data_->v.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_Q_IDX_B, mesh_data_->q_idx.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_T_IDX_B, mesh_data_->t_idx.bo);

            glDispatchComputeIndirect((long)NULL);
            glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
        }
        glUseProgram(0);

        /*
         * COPY PASS
         * - Reads the number of primitive written in previous Compute Pass
         * - Write the number of instances in the Draw Command Buffer
         * - Write the number of workgroups in the Dispatch Command Buffer
         */
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

//         commands_->PrintWGCountInDispatch();
//         commands_->PrintAtomicArray();

        glDisable(GL_RASTERIZER_DISCARD);
RENDER_PASS:
        if (settings.map_primcount) {
            drawn_node_count = commands_->GetDrawnNodeCount();
            full_node_count_ = commands_->GetFullNodeCount();

        }
        /*
         * RENDER PASS
         * - Reads the updated keys that did not get culled
         * - Performs the morphing
         * - Render the triangles
         */
        glEnable(GL_DEPTH_TEST);
        glFrontFace(GL_CCW);
//        glEnable(GL_CULL_FACE);

        glClearDepth(1.0);
        glClearColor(1,1,1,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(render_program_);
        {
            djgc_start(render_clock);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NODES_IN_B, nodes_bo_[ssbo_idx_.write_culled]);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_V_B, mesh_data_->v.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_Q_IDX_B, mesh_data_->q_idx.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_T_IDX_B, mesh_data_->t_idx.bo);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, transfo_->bo);
            commands_->BindForRender();
            glBindVertexArray(leaf_geometry.vao);
            glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
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
