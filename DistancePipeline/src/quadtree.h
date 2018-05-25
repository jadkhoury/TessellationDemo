#ifndef QUADTREE_H
#define QUADTREE_H
#include <vector>
#include <glm/gtx/string_cast.hpp>
#include "commands.h"
#include <glm/gtc/matrix_transform.hpp>
#include "utility.h"
#define DJ_OPENGL_IMPLEMENTATION 1
#include "dj_opengl.h"
#define DJ_ALGEBRA_IMPLEMENTATION 1
#include "dj_algebra.h"

//enum {QUADS, TRIANGLES, NUM_TYPES};
enum {LOD, WHITE, PRIMITIVES, NUM_COLOR_MODES} ColorModes;

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec4;
using glm::mat3;
using glm::mat4;

using namespace std;
using std::vector;


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
    BufferData v, idx;
    GLuint vao;
};

struct Mesh_Data
{
    const Vertex* v_array;
    uint* q_idx_array;
    uint* t_idx_array;

    BufferData v, q_idx, t_idx;
    int num_triangles, num_quads;
};

struct QuadtreeSettings {
    int uni_lvl = 0;
    float adaptive_factor = 0.7;
    bool uniform = false;
    bool triangle_mode = true;
    int prim_type = TRIANGLES;
    bool morph = true;
    float morph_k = 0.5;
    bool debug_morph = false;
    bool map_primcount = false;
    bool freeze = false;
    bool displace = true;
    int cpu_lod = 2;
    int color_mode = LOD;
    bool renderMVP = true;
} qts;


class QuadTree
{
public:
    // ****************** MEMBER VARIABLES ****************** //

    const int MAX_LVL = 10;
    const uint MAX_NUM_NODES = powOf2(2 * MAX_LVL);
    const GLsizei MAX_DATA_SIZE = MAX_NUM_NODES * sizeof(uvec4);
    const vec3 QUAD_CENTROID = vec3(0.5, 0.5, 1.0);
    const vec3 TRI_CENTROID = vec3(1.0/3.0, 1.0/3.0, 1.0);

    // Buffers and Arrays
    GLuint nodes_bo_[2];
    uvec4* nodes_array_;

    BufferCombo quad_leaf, triangle_leaf;

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

    uint num_prim_;
    vec3 prim_centroid_;

    Commands* commands_;
    uint offset_;

    djg_clock* clock;
    double tcpu, tgpu;

    mat4 model_mat_, view_mat_, projection_mat_;
    mat4 MVP_;
    vec3 cam_pos;
    bool transfo_updated;




    // ****************** MEMBER FUNCTIONS ****************** //
    // ---- math functions ---- //
    inline int powOf2(int exp)
    {
        return (1 << exp);
    }

    // ---- program functions ---- //
    void configureComputeProgram()
    {
        utility::SetUniformBool(compute_program_, "uniform_subdiv", qts.uniform);
        utility::SetUniformInt(compute_program_, "uniform_level", qts.uni_lvl);
        utility::SetUniformFloat(compute_program_, "adaptive_factor", qts.adaptive_factor);
        utility::SetUniformInt(compute_program_, "num_mesh_tri", mesh_->num_triangles);
        utility::SetUniformInt(compute_program_, "num_mesh_quad", mesh_->num_quads);


        if(qts.prim_type == QUADS){
            utility::SetUniformInt(compute_program_, "prim_type", QUADS);
            utility::SetUniformVec3(compute_program_, "prim_centroid", QUAD_CENTROID);
        } else if (qts.prim_type == TRIANGLES) {

            utility::SetUniformInt(compute_program_, "prim_type", TRIANGLES);
            utility::SetUniformVec3(compute_program_, "prim_centroid", TRI_CENTROID);
        }
    }

    void configureCullProgram()
    {

        utility::SetUniformBool(cull_program_, "uniform_subdiv", qts.uniform);
        utility::SetUniformInt(cull_program_, "uniform_level", qts.uni_lvl);
        utility::SetUniformFloat(cull_program_, "adaptive_factor", qts.adaptive_factor);
        utility::SetUniformInt(cull_program_, "num_mesh_tri", mesh_->num_triangles);
        utility::SetUniformInt(cull_program_, "num_mesh_quad", mesh_->num_quads);

        if(qts.prim_type == QUADS){
            utility::SetUniformInt(cull_program_, "prim_type", QUADS);
            utility::SetUniformVec3(cull_program_, "prim_centroid", QUAD_CENTROID);
        } else if (qts.prim_type == TRIANGLES) {

            utility::SetUniformInt(cull_program_, "prim_type", TRIANGLES);
            utility::SetUniformVec3(cull_program_, "prim_centroid", TRI_CENTROID);
        }
    }

    void configureRenderProgram()
    {
        utility::SetUniformBool(render_program_, "render_MVP", qts.renderMVP);
        utility::SetUniformFloat(render_program_, "adaptive_factor", qts.adaptive_factor);
        utility::SetUniformBool(render_program_, "morph", qts.morph);
        utility::SetUniformBool(render_program_, "debug_morph", qts.debug_morph);
        utility::SetUniformFloat(render_program_, "morph_k", qts.morph_k);
        utility::SetUniformBool(render_program_, "heightmap", qts.displace);
        utility::SetUniformInt(render_program_, "cpu_lod", qts.cpu_lod);
        utility::SetUniformInt(render_program_, "color_mode", qts.color_mode);

        if(qts.prim_type == QUADS){
            utility::SetUniformInt(render_program_, "prim_type", QUADS);
            utility::SetUniformVec3(render_program_, "prim_centroid", QUAD_CENTROID);
        } else if (qts.prim_type == TRIANGLES) {
            utility::SetUniformInt(render_program_, "prim_type", TRIANGLES);
            utility::SetUniformVec3(render_program_, "prim_centroid", TRI_CENTROID);
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



    // Allows the algorithm to alway subdiv in the hypothenus direction
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

    // ---- Mesh buffer functions ---- //
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

        mesh_->num_quads = mesh_->q_idx.count / 4;
        mesh_->num_triangles = mesh_->t_idx.count / 3;

        cout << "Mesh has " << mesh_->num_quads << " quads, "
             << mesh_->num_triangles << " triangles " << endl;

        return (glGetError() == GL_NO_ERROR);
    }

    // ---- buffer functions ---- //

    bool loadNodesBuffers()
    {
        nodes_array_ = new uvec4[MAX_NUM_NODES];
        if(qts.prim_type == TRIANGLES) {
            for (int ctr = 0; ctr < mesh_->num_triangles; ++ctr) {
                nodes_array_[ctr] = uvec4(0, 0x1, uint(ctr*3), 0);
            }
        } else if (qts.prim_type == QUADS) {
            for (int ctr = 0; ctr < mesh_->num_quads; ++ctr) {
                nodes_array_[ctr] = uvec4(0, 0x1, uint(ctr*4), 0);
            }
        }
        if(glIsBuffer(nodes_bo_[0]))
            glDeleteBuffers(1, &nodes_bo_[0]);
        if(glIsBuffer(nodes_bo_[1]))
            glDeleteBuffers(1, &nodes_bo_[1]);
        glCreateBuffers(2, nodes_bo_);
        glNamedBufferStorage(nodes_bo_[0], MAX_DATA_SIZE, nodes_array_, 0);
        glNamedBufferStorage(nodes_bo_[1], MAX_DATA_SIZE, nodes_array_, 0);
        return (glGetError() == GL_NO_ERROR);
    }



    uvec3 tr(uint elem, uint num_col, uint orientation){
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

    }

    vector<vec3> getTrianglePrimVertices(uint level)
    {
        vector<vec3> vertices;
        uint num_row = 1 << level;
        uint col = 0, row = 0;
        float d = 1.0 / float (num_row);
        while(row <= num_row)
        {
            while (col <= row)
            {
                vertices.push_back(vec3(col * d, 1.0 - row * d, 1.0));
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
        while(row < num_row)
        {
            orientation = (row % 2 == 0) ? 0 : 2;
            while (col < num_col)
            {
                indices.push_back(tr(elem, num_col, orientation));
                orientation = (orientation + 1) % 4;
                if(col > 0){
                    indices.push_back(tr(elem, num_col, orientation));
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

    bool loadTriangleLeafBuffers(uint level)
    {
        vector<vec3> vertices = getTrianglePrimVertices(level);
        vector<uvec3> indices = getTrianglePrimIndices(level);

        triangle_leaf.v.count = vertices.size();
        triangle_leaf.v.size = triangle_leaf.v.count * sizeof(vec3);
        if(glIsBuffer(triangle_leaf.v.bo)){
            glDeleteBuffers(1, &triangle_leaf.v.bo);
            triangle_leaf.v.bo = 0;
        }
        glCreateBuffers(1, &triangle_leaf.v.bo);
        glNamedBufferStorage(triangle_leaf.v.bo, triangle_leaf.v.size, &vertices[0], 0);

        triangle_leaf.idx.count = indices.size() * 3;
        triangle_leaf.idx.size = indices.size() * sizeof(uvec3);
        if(glIsBuffer(triangle_leaf.idx.bo)){
            glDeleteBuffers(1, &triangle_leaf.idx.bo);
            triangle_leaf.idx.bo = 0;
        }
        glCreateBuffers(1, &triangle_leaf.idx.bo);
        glNamedBufferStorage(triangle_leaf.idx.bo, triangle_leaf.idx.size, &indices[0], 0);

        cout << triangle_leaf.v.count   << " triangle vertices (" << triangle_leaf.v.size << "B)" << endl;
        cout << triangle_leaf.idx.count << " triangle indices  (" << triangle_leaf.idx.size << "B)" << endl;

        return (glGetError() == GL_NO_ERROR);
    }

    bool loadQuadLeafBuffers(uint level)
    {
        const void *data;
        int vertexCnt, indexCnt;
        int lvl = (0x1 << level) - 1;
        djg_mesh* mesh = djgm_load_plane(lvl, lvl);

        const djgm_vertex* vertices = djgm_get_vertices(mesh, &vertexCnt);
        quad_leaf.v.count = vertexCnt;
        quad_leaf.v.size = quad_leaf.v.count * sizeof(vec3);
        vec3* pos = new vec3[quad_leaf.v.count];
        for (uint i = 0; i < quad_leaf.v.count; ++i) {
            pos[i] = vec3(vertices[i].st.s, vertices[i].st.t, 1.0);
        }
        if(glIsBuffer(quad_leaf.v.bo))
            glDeleteBuffers(1, &quad_leaf.v.bo);
        glCreateBuffers(1, &quad_leaf.v.bo);
        glNamedBufferStorage(quad_leaf.v.bo, quad_leaf.v.size, pos, 0);

        data = (const void *)djgm_get_triangles(mesh, &indexCnt);
        quad_leaf.idx.count = indexCnt;
        quad_leaf.idx.size = quad_leaf.idx.count * sizeof(uint16_t);
        if(glIsBuffer(quad_leaf.idx.bo))
            glDeleteBuffers(1, &quad_leaf.idx.bo);
        glCreateBuffers(1, &quad_leaf.idx.bo);
        glNamedBufferStorage(quad_leaf.idx.bo, quad_leaf.idx.size, data, 0);

        cout << quad_leaf.v.count   << " quad vertices ("  << quad_leaf.v.size << "B)" << endl;
        cout << quad_leaf.idx.count << " quad indices  (" << quad_leaf.idx.size << "B)" << endl;


        djgm_release(mesh);

        return (glGetError() == GL_NO_ERROR);
    }

    // ---- VAO functions ---- //
    bool loadTriangleLeafVao()
    {
        if(glIsVertexArray(triangle_leaf.vao)) {
            glDeleteVertexArrays(1, &triangle_leaf.vao);
            triangle_leaf.vao = 0;
        }
        glCreateVertexArrays(1, &triangle_leaf.vao);
        glVertexArrayAttribBinding(triangle_leaf.vao, 1, 0);
        glVertexArrayAttribFormat(triangle_leaf.vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(triangle_leaf.vao, 1);
        glVertexArrayVertexBuffer(triangle_leaf.vao, 0, triangle_leaf.v.bo, 0, sizeof(vec3));
        glVertexArrayElementBuffer(triangle_leaf.vao, triangle_leaf.idx.bo);


        return (glGetError() == GL_NO_ERROR);
    }

    bool loadQuadLeafVao()
    {
        if(glIsVertexArray(quad_leaf.vao))
            glDeleteVertexArrays(1, &quad_leaf.vao);

        glCreateVertexArrays(1, &quad_leaf.vao);
        glVertexArrayAttribBinding(quad_leaf.vao, 0, 0);
        glVertexArrayAttribFormat(quad_leaf.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(quad_leaf.vao, 0);
        glVertexArrayVertexBuffer(quad_leaf.vao, 0, quad_leaf.v.bo, 0, sizeof(vec3));
        glVertexArrayElementBuffer(quad_leaf.vao, quad_leaf.idx.bo);
        return (glGetError() == GL_NO_ERROR);
    }

    // ---- pingpong functions ---- //
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
    QuadtreeSettings* Init(Mesh_Data* m_data)
    {
        cout << "-> QUADTREE" << endl;
        workgroup_size_ = vec3(256, 1, 1);
        num_workgroup_ = ceil(MAX_NUM_NODES / (workgroup_size_.x * workgroup_size_.y * workgroup_size_.z));
        num_prim_ = 0;
        commands_ = new Commands();
        clock = djgc_create();
        mesh_ = m_data;

        if(!loadPrograms())
            throw std::runtime_error("shader creation error");

        loadMeshBuffers();
        loadQuadLeafBuffers(qts.cpu_lod);
        loadTriangleLeafBuffers(qts.cpu_lod);
        loadQuadLeafVao();
        loadTriangleLeafVao();
        loadNodesBuffers();
        commands_->Init(quad_leaf.idx.count, triangle_leaf.idx.count, num_workgroup_);
        init_pingpong();

        ReconfigureShaders();

        glUseProgram(0);

        cout << "-> END QUADTREE" << endl << endl;

        return &qts;
    }

    void Draw(float deltaT)
    {
        if(transfo_updated) {
            UploadTransforms();
            transfo_updated = false;
        }

        if(!qts.freeze)
        {
            // ** Compute Pass ** //
            djgc_start(clock);

            glEnable(GL_RASTERIZER_DISCARD);

            glUseProgram(compute_program_);
            {
                utility::SetUniformFloat(compute_program_, "deltaT", deltaT);
                //pingpong();
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, read_ssbo_);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, write_ssbo_);
                commands_->BindForCompute(2, 3, qts.prim_type);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mesh_->v.bo);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mesh_->q_idx.bo);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, mesh_->t_idx.bo);

                glDispatchComputeIndirect((long)NULL);
                glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

            }
            glUseProgram(0);

            if(qts.map_primcount){
                num_prim_ = commands_->getPrimCount();
            }

            // ** Cull Pass ** //
            glUseProgram(cull_program_);
            {
                pingpong();
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, read_ssbo_);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, write_ssbo_);
                commands_->BindForCull(2, 3, qts.prim_type);
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
//        glViewport(256, 0, 1024, 1024);
        glUseProgram(render_program_);
        {
            glViewport(512, 0, 1024, 1024);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, write_ssbo_);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mesh_->v.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mesh_->q_idx.bo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, mesh_->t_idx.bo);
            offset_ = commands_->BindForRender(qts.prim_type);
            if(qts.prim_type == QUADS) {
                glBindVertexArray(quad_leaf.vao);
                glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, BUFFER_OFFSET(offset_));

            } else if(qts.prim_type == TRIANGLES){
                glBindVertexArray(triangle_leaf.vao);
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
        glDeleteBuffers(1, &quad_leaf.v.bo);
        glDeleteBuffers(1, &quad_leaf.idx.bo);
        glDeleteBuffers(1, &triangle_leaf.v.bo);
        glDeleteBuffers(1, &triangle_leaf.idx.bo);
        glDeleteVertexArrays(1, &triangle_leaf.vao);
        glDeleteVertexArrays(1, &quad_leaf.vao);
        commands_->Cleanup();
        delete[] nodes_array_;
    }

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
        loadQuadLeafBuffers(qts.cpu_lod);
        loadTriangleLeafBuffers(qts.cpu_lod);
        loadQuadLeafVao();
        loadTriangleLeafVao();
        commands_->ReinitializeCommands(quad_leaf.idx.count, triangle_leaf.idx.count, num_workgroup_);
        loadNodesBuffers();
        configureRenderProgram();

    }


    void SetPrimType(int i)
    {
        qts.prim_type = i;
        ReinitQuadTree();
    }

    void GetTicks(double& cpu, double& gpu)
    {
        cpu = tcpu;
        gpu = tgpu;
    }

    int GetPrimcount()
    {
        return num_prim_;
    }

    void UpdateTransforms(const mat4& M, const mat4& V, const mat4& P, const vec3& cp)
    {
        model_mat_ = M;
        view_mat_ = V;
        projection_mat_ = P;
        cam_pos = cp;
        MVP_ = projection_mat_ * view_mat_ * model_mat_;
        transfo_updated = true;
    }

    void UpdateTransforms(const mat4& V, const mat4& P, const vec3& cp)
    {
        view_mat_ = V;
        projection_mat_ = P;
        cam_pos = cp;
        MVP_ = projection_mat_ * view_mat_ * model_mat_;
        transfo_updated = true;
    }


    mat4 GetModel(){
        return model_mat_;
    }

    void SetModel(const mat4& new_M)
    {
        model_mat_ = new_M;
        MVP_ = projection_mat_ * view_mat_ * model_mat_;
        transfo_updated = true;
    }
    void UpdateColorMode()
    {
        utility::SetUniformInt(render_program_, "color_mode", qts.color_mode);
    }

    void UploadTransforms()
    {
        utility::SetUniformMat4(compute_program_, "M", model_mat_);
        utility::SetUniformMat4(compute_program_, "V", view_mat_);
        utility::SetUniformMat4(compute_program_, "P", projection_mat_);
        utility::SetUniformMat4(compute_program_, "MVP", MVP_);
        utility::SetUniformVec3(compute_program_, "cam_pos", cam_pos);

        utility::SetUniformMat4(cull_program_, "M", model_mat_);
        utility::SetUniformMat4(cull_program_, "V", view_mat_);
        utility::SetUniformMat4(cull_program_, "P", projection_mat_);
        utility::SetUniformMat4(cull_program_, "MVP", MVP_);
        utility::SetUniformVec3(cull_program_, "cam_pos", cam_pos);

        utility::SetUniformMat4(render_program_, "M", model_mat_);
        utility::SetUniformMat4(render_program_, "V", view_mat_);
        utility::SetUniformMat4(render_program_, "P", projection_mat_);
        utility::SetUniformMat4(render_program_, "MVP", MVP_);
        if(!qts.freeze)
            utility::SetUniformVec3(render_program_, "cam_pos", cam_pos);
    }
};

#endif
