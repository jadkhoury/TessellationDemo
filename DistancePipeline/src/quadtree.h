#ifndef QUADTREE_H
#define QUADTREE_H

#include "commands.h"
#include "common.h"

struct QuadtreeSettings {
    int uni_lvl;
    float adaptive_factor;
    bool uniform;
    bool triangle_mode;
    int prim_type;
    bool morph;
    float morph_k;
    bool debug_morph;
    bool map_primcount;
    bool freeze;
    bool displace;
    int cpu_lod;
    int color_mode;
    bool renderMVP;
};

class QuadTree
{
private:
    const vec3 QUAD_CENTROID = vec3(0.5, 0.5, 1.0);
    const vec3 TRIANGLE_CENTROID = vec3(1.0/3.0, 1.0/3.0, 1.0);

    // Buffers and Arrays
    GLuint nodes_bo_[2];
    uvec4* nodes_array_;
    GLuint max_num_nodes_;
    GLsizei max_ssbo_size_;

    BufferCombo quad_leaf_, triangle_leaf_;

    // Mesh data
    Mesh_Data* mesh_;

    //Ping pong variables
    int read_from_idx_;
    GLuint read_ssbo_, write_ssbo_;

    //Programs
    GLuint render_program_, compute_program_, cull_program_;

    //Compute Shader parameters
    int num_workgroup_;
    vec3 workgroup_size_;

    vec3 prim_centroid_;

    Commands* commands_;
    uint offset_;

public:
    djg_clock* clock;
    double tcpu, tgpu;

    mat4 model_mat, view_mat, projection_mat;
    mat4 MVP;
    vec3 cam_pos;
    bool transfo_updated;

    uint prim_count;
    QuadtreeSettings settings;

private:

    inline int powOf2(int exp) {  return (1 << exp); }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Shader Management Function
    ///

    void configureComputeProgram()
    {
        utility::SetUniformBool(compute_program_, "uniform_subdiv", settings.uniform);
        utility::SetUniformInt(compute_program_, "uniform_level", settings.uni_lvl);
        utility::SetUniformFloat(compute_program_, "adaptive_factor", settings.adaptive_factor);
        utility::SetUniformInt(compute_program_, "num_mesh_tri", mesh_->triangle_count);
        utility::SetUniformInt(compute_program_, "num_mesh_quad", mesh_->quad_count);


        if(settings.prim_type == QUADS){
            utility::SetUniformInt(compute_program_, "prim_type", QUADS);
            utility::SetUniformVec3(compute_program_, "prim_centroid", QUAD_CENTROID);
        } else if (settings.prim_type == TRIANGLES) {

            utility::SetUniformInt(compute_program_, "prim_type", TRIANGLES);
            utility::SetUniformVec3(compute_program_, "prim_centroid", TRIANGLE_CENTROID);
        }
    }

    void configureCullProgram()
    {

        utility::SetUniformBool(cull_program_, "uniform_subdiv", settings.uniform);
        utility::SetUniformInt(cull_program_, "uniform_level", settings.uni_lvl);
        utility::SetUniformFloat(cull_program_, "adaptive_factor", settings.adaptive_factor);
        utility::SetUniformInt(cull_program_, "num_mesh_tri", mesh_->triangle_count);
        utility::SetUniformInt(cull_program_, "num_mesh_quad", mesh_->quad_count);

        if(settings.prim_type == QUADS){
            utility::SetUniformInt(cull_program_, "prim_type", QUADS);
            utility::SetUniformVec3(cull_program_, "prim_centroid", QUAD_CENTROID);
        } else if (settings.prim_type == TRIANGLES) {

            utility::SetUniformInt(cull_program_, "prim_type", TRIANGLES);
            utility::SetUniformVec3(cull_program_, "prim_centroid", TRIANGLE_CENTROID);
        }
    }

    void configureRenderProgram()
    {
        utility::SetUniformBool(render_program_, "render_MVP", settings.renderMVP);
        utility::SetUniformFloat(render_program_, "adaptive_factor", settings.adaptive_factor);
        utility::SetUniformBool(render_program_, "morph", settings.morph);
        utility::SetUniformBool(render_program_, "debug_morph", settings.debug_morph);
        utility::SetUniformFloat(render_program_, "morph_k", settings.morph_k);
        utility::SetUniformBool(render_program_, "heightmap", settings.displace);
        utility::SetUniformInt(render_program_, "cpu_lod", settings.cpu_lod);
        utility::SetUniformInt(render_program_, "color_mode", settings.color_mode);

        if(settings.prim_type == QUADS){
            utility::SetUniformInt(render_program_, "prim_type", QUADS);
            utility::SetUniformVec3(render_program_, "prim_centroid", QUAD_CENTROID);
        } else if (settings.prim_type == TRIANGLES) {
            utility::SetUniformInt(render_program_, "prim_type", TRIANGLES);
            utility::SetUniformVec3(render_program_, "prim_centroid", TRIANGLE_CENTROID);
        }
    }

    bool loadComputeProgram()
    {
        cout << "---> Loading Compute Program" << endl;
        if(!glIsProgram(compute_program_))
            compute_program_ = 0;
        djg_program *djp = djgp_create();
        djgp_push_string(djp, "#define TRIANGLES %i\n", TRIANGLES);
        djgp_push_string(djp, "#define QUADS %i\n", QUADS);

        char buf[1024];
        djgp_push_file(djp, strcat2(buf, shader_dir, "ltree_jk.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "ltree_common.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "quadtree_cs.glsl"));
        if (!djgp_to_gl(djp, 450, false, true, &compute_program_))
        {
            djgp_release(djp);
            cout << "-----> Failure" << endl;

            return false;
        }
        djgp_release(djp);
        cout << "-----> Success" << endl;
        configureComputeProgram();
        return (glGetError() == GL_NO_ERROR);
    }

    bool loadCullProgram()
    {
        cout << "---> Loading Cull Program" << endl;

        if(!glIsProgram(cull_program_))
            cull_program_ = 0;
        djg_program *djp = djgp_create();
        djgp_push_string(djp, "#define TRIANGLES %i\n", TRIANGLES);
        djgp_push_string(djp, "#define QUADS %i\n", QUADS);

        char buf[1024];

        djgp_push_file(djp, strcat2(buf, shader_dir, "ltree_jk.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "ltree_common.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "dj_frustum.glsl"));
        djgp_push_file(djp, strcat2(buf, shader_dir, "quadtree_cull.glsl"));

        if (!djgp_to_gl(djp, 450, false, true, &cull_program_))
        {
            djgp_release(djp);
            cout << "-----> Failure" << endl;
            return false;
        }
        djgp_release(djp);
        cout << "-----> Success" << endl;
        configureCullProgram();
        return (glGetError() == GL_NO_ERROR);
    }

    bool loadRenderProgram()
    {
        cout << "---> Loading Render Program" << endl;

        if(!glIsProgram(render_program_))
            render_program_ = 0;
        djg_program *djp = djgp_create();
        djgp_push_string(djp, "#define TRIANGLES %i\n", TRIANGLES);
        djgp_push_string(djp, "#define QUADS %i\n", QUADS);
        djgp_push_string(djp, "#define LOD %i\n", LOD);
        djgp_push_string(djp, "#define WHITE %i\n", WHITE);
        djgp_push_string(djp, "#define PRIMITIVES %i\n", PRIMITIVES);



        char buf[1024];

        djgp_push_file(djp, strcat2(buf,shader_dir, "gpu_noise_lib.glsl"));
        djgp_push_file(djp, strcat2(buf,shader_dir, "ltree_jk.glsl"));
        djgp_push_file(djp, strcat2(buf,shader_dir, "ltree_common.glsl"));
        djgp_push_file(djp, strcat2(buf,shader_dir, "dj_heightmap.glsl"));
        djgp_push_file(djp, strcat2(buf,shader_dir, "quadtree_render.glsl"));
        if (!djgp_to_gl(djp, 450, false, true, &render_program_))
        {
            djgp_release(djp);
            cout << "-----> Failure" << endl;
            return false;
        }
        djgp_release(djp);
        cout << "-----> Success" << endl;
        configureRenderProgram();
        return (glGetError() == GL_NO_ERROR);
    }

    bool loadPrograms()
    {
        bool v = true;
        v &= loadComputeProgram();
        v &= loadCullProgram();
        v &= loadRenderProgram();
        UploadTransforms();
        return v;
    }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Leaf & Mesh Geometry Functions
    ///

    void reorderIndices(uint* i_array, const Vertex* v, uint count){
        uint  i0, i1, i2;
        float d01, d02, d12, max_d;
        for(uint i=0; i < count; i+= 3)
        {
            i0 = i_array[i + 0];
            i1 = i_array[i + 1];
            i2 = i_array[i + 2];

            d01 = glm::distance(v[i0].p, v[i1].p);
            d02 = glm::distance(v[i0].p, v[i2].p);
            d12 = glm::distance(v[i1].p, v[i2].p);

            max_d = max(d01, max(d02, d12));
            if(max_d == d01){
                i_array[i + 0] = i2;
                i_array[i + 1] = i0;
                i_array[i + 2] = i1;
            } else if (max_d == d02) {
                i_array[i + 0] = i1;
                i_array[i + 1] = i0;
                i_array[i + 2] = i2;
            }
        }
    }

    vector<vec2> getTrianglePrimVertices(uint level)
    {
        vector<vec2> vertices;
        uint num_row = 1 << level;
        uint col = 0, row = 0;
        float d = 1.0 / float (num_row);
        while(row <= num_row)
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

    vector<uvec3> getTrianglePrimIndices(uint level)
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

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Buffer Management Functions
    ///

    bool loadMeshBuffers()
    {
        if(glIsBuffer(mesh_->v.bo))
            glDeleteBuffers(1, &(mesh_->v.bo));
        glCreateBuffers(1, &(mesh_->v.bo));
        glNamedBufferStorage(mesh_->v.bo,
                             sizeof(Vertex)*mesh_->v.count,
                             (const void*)(mesh_->v_array),
                             0);

        if(glIsBuffer(mesh_->q_idx.bo))
            glDeleteBuffers(1, &(mesh_->q_idx.bo));
        glCreateBuffers(1, &(mesh_->q_idx.bo));
        glNamedBufferStorage(mesh_->q_idx.bo,
                             sizeof(uint) * mesh_->q_idx.count,
                             (const void*)(mesh_->q_idx_array),
                             0);

        reorderIndices(mesh_->t_idx_array, mesh_->v_array, mesh_->t_idx.count);
        if(glIsBuffer(mesh_->t_idx.bo))
            glDeleteBuffers(1, &(mesh_->t_idx.bo));
        glCreateBuffers(1, &(mesh_->t_idx.bo));
        glNamedBufferStorage(mesh_->t_idx.bo,
                             sizeof(uint) * mesh_->t_idx.count,
                             (const void*)(mesh_->t_idx_array),
                             0);

        mesh_->quad_count = mesh_->q_idx.count / 4;
        mesh_->triangle_count = mesh_->t_idx.count / 3;

        cout << "Mesh has " << mesh_->quad_count << " quads, "
             << mesh_->triangle_count << " triangles " << endl;

        return (glGetError() == GL_NO_ERROR);
    }

    bool loadNodesBuffers()
    {
        int max_ssbo_size;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_ssbo_size);
        max_ssbo_size /= 8;
        int max_num_nodes = max_ssbo_size / sizeof(uvec4);
        printf("max_num_nodes: %s \n", LongToString(max_num_nodes).c_str());
        printf("max_ssbo_size: %s \n", LongToString(max_ssbo_size).c_str());


        if(settings.prim_type == TRIANGLES) {
            nodes_array_ = new uvec4[max_num_nodes];
            for (int ctr = 0; ctr < mesh_->triangle_count; ++ctr) {
                nodes_array_[ctr] = uvec4(0, 0x1, uint(ctr*3), 0);
            }
        } else if (settings.prim_type == QUADS) {
            nodes_array_ = new uvec4[max_num_nodes];
            for (int ctr = 0; ctr < mesh_->quad_count; ++ctr) {
                nodes_array_[ctr] = uvec4(0, 0x1, uint(ctr*4), 0);
            }
        }
        if(glIsBuffer(nodes_bo_[0]))
            glDeleteBuffers(1, &nodes_bo_[0]);
        if(glIsBuffer(nodes_bo_[1]))
            glDeleteBuffers(1, &nodes_bo_[1]);

        glCreateBuffers(2, nodes_bo_);
        glNamedBufferStorage(nodes_bo_[0], max_ssbo_size, nodes_array_, 0);
        glNamedBufferStorage(nodes_bo_[1], max_ssbo_size, nodes_array_, 0);
        return (glGetError() == GL_NO_ERROR);
    }

    bool loadTriangleLeafBuffers(uint level)
    {
        vector<vec2> vertices = getTrianglePrimVertices(level);
        vector<uvec3> indices = getTrianglePrimIndices(level);

        triangle_leaf_.v.count = vertices.size();
        triangle_leaf_.v.size = triangle_leaf_.v.count * sizeof(vec2);
        if(glIsBuffer(triangle_leaf_.v.bo)){
            glDeleteBuffers(1, &triangle_leaf_.v.bo);
            triangle_leaf_.v.bo = 0;
        }
        glCreateBuffers(1, &triangle_leaf_.v.bo);
        glNamedBufferStorage(triangle_leaf_.v.bo, triangle_leaf_.v.size, &vertices[0], 0);

        triangle_leaf_.idx.count = indices.size() * 3;
        triangle_leaf_.idx.size = indices.size() * sizeof(uvec3);
        if(glIsBuffer(triangle_leaf_.idx.bo)){
            glDeleteBuffers(1, &triangle_leaf_.idx.bo);
            triangle_leaf_.idx.bo = 0;
        }
        glCreateBuffers(1, &triangle_leaf_.idx.bo);
        glNamedBufferStorage(triangle_leaf_.idx.bo, triangle_leaf_.idx.size, &indices[0], 0);

        cout << triangle_leaf_.v.count   << " triangle vertices (" << triangle_leaf_.v.size << "B)" << endl;
        cout << triangle_leaf_.idx.count << " triangle indices  (" << triangle_leaf_.idx.size << "B)" << endl;

        return (glGetError() == GL_NO_ERROR);
    }

    bool loadQuadLeafBuffers(uint level)
    {
        const void *data;
        int vertexCnt, indexCnt;
        int lvl = (0x1 << level) - 1;
        djg_mesh* mesh = djgm_load_plane(lvl, lvl);

        const djgm_vertex* vertices = djgm_get_vertices(mesh, &vertexCnt);
        quad_leaf_.v.count = vertexCnt;
        quad_leaf_.v.size = quad_leaf_.v.count * sizeof(vec2);
        vec2* pos = new vec2[quad_leaf_.v.count];
        for (uint i = 0; i < quad_leaf_.v.count; ++i) {
            pos[i] = vec2(vertices[i].st.s, vertices[i].st.t);
        }
        if(glIsBuffer(quad_leaf_.v.bo))
            glDeleteBuffers(1, &quad_leaf_.v.bo);
        glCreateBuffers(1, &quad_leaf_.v.bo);
        glNamedBufferStorage(quad_leaf_.v.bo, quad_leaf_.v.size, pos, 0);

        data = (const void *)djgm_get_triangles(mesh, &indexCnt);
        quad_leaf_.idx.count = indexCnt;
        quad_leaf_.idx.size = quad_leaf_.idx.count * sizeof(uint16_t);
        if(glIsBuffer(quad_leaf_.idx.bo))
            glDeleteBuffers(1, &quad_leaf_.idx.bo);
        glCreateBuffers(1, &quad_leaf_.idx.bo);
        glNamedBufferStorage(quad_leaf_.idx.bo, quad_leaf_.idx.size, data, 0);

        cout << quad_leaf_.v.count   << " quad vertices ("  << quad_leaf_.v.size << "B)" << endl;
        cout << quad_leaf_.idx.count << " quad indices  (" << quad_leaf_.idx.size << "B)" << endl;

        djgm_release(mesh);

        return (glGetError() == GL_NO_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// VAO Management Functions
    ///

    bool loadTriangleLeafVao()
    {
        if(glIsVertexArray(triangle_leaf_.vao)) {
            glDeleteVertexArrays(1, &triangle_leaf_.vao);
            triangle_leaf_.vao = 0;
        }
        glCreateVertexArrays(1, &triangle_leaf_.vao);
        glVertexArrayAttribBinding(triangle_leaf_.vao, 1, 0);
        glVertexArrayAttribFormat(triangle_leaf_.vao, 1, 2, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(triangle_leaf_.vao, 1);
        glVertexArrayVertexBuffer(triangle_leaf_.vao, 0, triangle_leaf_.v.bo, 0, sizeof(vec2));
        glVertexArrayElementBuffer(triangle_leaf_.vao, triangle_leaf_.idx.bo);


        return (glGetError() == GL_NO_ERROR);
    }

    bool loadQuadLeafVao()
    {
        if(glIsVertexArray(quad_leaf_.vao))
            glDeleteVertexArrays(1, &quad_leaf_.vao);

        glCreateVertexArrays(1, &quad_leaf_.vao);
        glVertexArrayAttribBinding(quad_leaf_.vao, 0, 0);
        glVertexArrayAttribFormat(quad_leaf_.vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(quad_leaf_.vao, 0);
        glVertexArrayVertexBuffer(quad_leaf_.vao, 0, quad_leaf_.v.bo, 0, sizeof(vec2));
        glVertexArrayElementBuffer(quad_leaf_.vao, quad_leaf_.idx.bo);
        return (glGetError() == GL_NO_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// PingPong Functions
    ///

    void pingpong()
    {
        read_from_idx_ = 1 - read_from_idx_;
        read_ssbo_ = nodes_bo_[read_from_idx_];
        write_ssbo_ = nodes_bo_[1 - read_from_idx_];
    }

    void init_pingpong()
    {
        read_from_idx_ = 0;
        read_ssbo_ = nodes_bo_[read_from_idx_];
        write_ssbo_ = nodes_bo_[1 - read_from_idx_];
    }

public:

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// RUN Functions
    ///

    void Init(Mesh_Data* m_data, const QuadtreeSettings& init_settings)
    {
        cout << "-> QUADTREE" << endl;

        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_ssbo_size_);
        max_ssbo_size_ /= 8;
        max_num_nodes_ = max_ssbo_size_ / sizeof(uvec4);
        printf("max_num_nodes: %s \n", LongToString(max_num_nodes_).c_str());
        printf("max_ssbo_size: %s \n", LongToString(max_ssbo_size_).c_str());

        settings = init_settings;
        workgroup_size_ = vec3(256, 1, 1);
        num_workgroup_ = ceil(max_num_nodes_ / (workgroup_size_.x * workgroup_size_.y * workgroup_size_.z));
        prim_count = 0;
        commands_ = new Commands();
        clock = djgc_create();
        mesh_ = m_data;

        if(!loadPrograms())
            throw std::runtime_error("shader creation error");

        loadMeshBuffers();
        loadQuadLeafBuffers(settings.cpu_lod);
        loadTriangleLeafBuffers(settings.cpu_lod);
        loadQuadLeafVao();
        loadTriangleLeafVao();
        loadNodesBuffers();
        commands_->Init(quad_leaf_.idx.count, triangle_leaf_.idx.count, num_workgroup_);
        init_pingpong();

        ReconfigureShaders();

        glUseProgram(0);

        cout << "-> END QUADTREE" << endl << endl;
    }

    void Draw(float deltaT)
    {
        if(transfo_updated) {
            UploadTransforms();
            transfo_updated = false;
        }

        if(!settings.freeze)
        {
            // ** Compute Pass ** //
            djgc_start(clock);

            glEnable(GL_RASTERIZER_DISCARD);

            glUseProgram(compute_program_);
            {
                utility::SetUniformFloat(compute_program_, "deltaT", deltaT);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, read_ssbo_);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, write_ssbo_);
                commands_->BindForCompute(2, 3, settings.prim_type);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mesh_->v.bo);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mesh_->q_idx.bo);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, mesh_->t_idx.bo);

                glDispatchComputeIndirect((long)NULL);
                glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

            }
            glUseProgram(0);

            if(settings.map_primcount){
                prim_count = commands_->getPrimCount();
            }

            // ** Cull Pass ** //
            glUseProgram(cull_program_);
            {
                pingpong();
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, read_ssbo_);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, write_ssbo_);
                commands_->BindForCull(2, 3, settings.prim_type);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mesh_->v.bo);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mesh_->q_idx.bo);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, mesh_->t_idx.bo);
                glDispatchComputeIndirect((long)NULL);
                glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
            }
            glUseProgram(0);

            glDisable(GL_RASTERIZER_DISCARD);
        }

        // ** Draw Pass ** //
        glEnable(GL_DEPTH_TEST);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(render_program_);
        {
            glViewport(512, 0, 1024, 1024);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, write_ssbo_);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mesh_->v.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mesh_->q_idx.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, mesh_->t_idx.bo);
            offset_ = commands_->BindForRender(settings.prim_type);
            if(settings.prim_type == QUADS) {
                glBindVertexArray(quad_leaf_.vao);
                glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, BUFFER_OFFSET(offset_));

            } else if(settings.prim_type == TRIANGLES){
                glBindVertexArray(triangle_leaf_.vao);
                glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, BUFFER_OFFSET(offset_));
            }
            glBindVertexArray(0);
        }
        glUseProgram(0);

        djgc_stop(clock);
        djgc_ticks(clock, &tcpu, &tgpu);
    }

    void CleanUp()
    {
        glUseProgram(0);
        glDeleteBuffers(2, nodes_bo_);
        glDeleteProgram(render_program_);
        glDeleteProgram(compute_program_);
        glDeleteBuffers(1, &quad_leaf_.v.bo);
        glDeleteBuffers(1, &quad_leaf_.idx.bo);
        glDeleteBuffers(1, &triangle_leaf_.v.bo);
        glDeleteBuffers(1, &triangle_leaf_.idx.bo);
        glDeleteVertexArrays(1, &triangle_leaf_.vao);
        glDeleteVertexArrays(1, &quad_leaf_.vao);
        commands_->Cleanup();
        delete[] nodes_array_;
    }

    ////////////////////////////////////////////////////////////////////////////
    ///
    /// Accessors & Reload Functions
    ///

    void ReloadShaders()
    {
        bool v = loadPrograms();
        if(v){
            ReinitQuadTree();
            cout << "** RELOAD SUCCESS **" << endl << endl;
        }else{
            cout << "** FAILED TO RELOAD **" << endl << endl;
        }

    }

    void ReconfigureShaders()
    {
        configureComputeProgram();
        configureCullProgram();
        configureRenderProgram();
    }

    void Reinitialize()
    {
        init_pingpong();
        loadMeshBuffers();
        loadNodesBuffers();
        configureComputeProgram();
        configureCullProgram();
        configureRenderProgram();
        commands_->ReloadCommands();
        transfo_updated = true;
    }

    void ReinitQuadTree()
    {
        cout << "Reinitializing Quadtree" << endl;
        init_pingpong();
        loadNodesBuffers();
        configureComputeProgram();
        configureCullProgram();
        configureRenderProgram();
        commands_->ReloadCommands();
        transfo_updated = true;
    }

    void ReloadPrimitives()
    {
        loadQuadLeafBuffers(settings.cpu_lod);
        loadTriangleLeafBuffers(settings.cpu_lod);
        loadQuadLeafVao();
        loadTriangleLeafVao();
        commands_->ReinitializeCommands(quad_leaf_.idx.count, triangle_leaf_.idx.count, num_workgroup_);
        loadNodesBuffers();
        configureRenderProgram();
    }


    void SetPrimType(int i)
    {
        settings.prim_type = i;
        ReinitQuadTree();
    }

    void UpdateTransforms(const mat4& M, const mat4& V, const mat4& P, const vec3& cp)
    {
        model_mat = M;
        view_mat = V;
        projection_mat = P;
        cam_pos = cp;
        MVP = projection_mat * view_mat * model_mat;
        transfo_updated = true;
    }

    void UpdateTransforms(const mat4& V, const mat4& P, const vec3& cp)
    {
        view_mat = V;
        projection_mat = P;
        cam_pos = cp;
        MVP = projection_mat * view_mat * model_mat;
        transfo_updated = true;
    }

    void SetModel(const mat4& new_M)
    {
        model_mat = new_M;
        MVP = projection_mat * view_mat * model_mat;
        transfo_updated = true;
    }
    void UpdateColorMode()
    {
        utility::SetUniformInt(render_program_, "color_mode", settings.color_mode);
    }

    void UploadTransforms()
    {
        utility::SetUniformMat4(compute_program_, "M", model_mat);
        utility::SetUniformMat4(compute_program_, "V", view_mat);
        utility::SetUniformMat4(compute_program_, "P", projection_mat);
        utility::SetUniformMat4(compute_program_, "MVP", MVP);
        utility::SetUniformVec3(compute_program_, "cam_pos", cam_pos);

        utility::SetUniformMat4(cull_program_, "M", model_mat);
        utility::SetUniformMat4(cull_program_, "V", view_mat);
        utility::SetUniformMat4(cull_program_, "P", projection_mat);
        utility::SetUniformMat4(cull_program_, "MVP", MVP);
        utility::SetUniformVec3(cull_program_, "cam_pos", cam_pos);

        utility::SetUniformMat4(render_program_, "M", model_mat);
        utility::SetUniformMat4(render_program_, "V", view_mat);
        utility::SetUniformMat4(render_program_, "P", projection_mat);
        utility::SetUniformMat4(render_program_, "MVP", MVP);
        if(!settings.freeze)
            utility::SetUniformVec3(render_program_, "cam_pos", cam_pos);
    }
};

#endif
