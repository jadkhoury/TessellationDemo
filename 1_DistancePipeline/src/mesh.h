#ifndef MESH_H
#define MESH_H

#include "common.h"
#include "quadtree.h"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec4;

#ifndef QUADTREE_H
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
    BufferData v;
    BufferData idx;
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
};
#endif

enum {
    TERRAIN,
    MESH,
    NUM_MODES
};

using namespace std;
class Mesh
{
private:
    Mesh_Data my_data;
    QuadTree* quadtree;
    uint mode;

    const QuadtreeSettings INIT_SETTINGS = {
        /* int uni_lvl */ 0,
        /* float adaptive_factor */ 50.0,
        /* bool uniform */ false,
        /* bool triangle_mode */ true,
        /* int prim_type */ TRIANGLES,
        /* bool morph */ true,
        /* float morph_k */ 0.0,
        /* bool debug_morph */ false,
        /* bool map_primcount */ true,
        /* bool freeze */ false,
        /* bool displace */ true,
        /* int cpu_lod */ 2,
        /* int color_mode */ LOD,
        /* bool renderMVP */ true,
    };

    const string obj_path = "../bigguy.obj";

    int roundUpToSq(int n){
        int sq = ceil(sqrt(n));
        return (sq * sq);
    }

    void loadGrid()
    {
        Vertex* vertices = new Vertex[4];
        vec4 sky(0.0, 0.0, 1.0, 1.0);
        float factor = 5.0;
        vec2 zeros(0,0);

        vertices[0] = { factor * vec4(-0.5,-0.5,0,1), sky, vec2(0,0), zeros };
        vertices[1] = { factor * vec4(-0.5, 0.5,0,1), sky, vec2(0,1), zeros };
        vertices[2] = { factor * vec4( 0.5,-0.5,0,1), sky, vec2(1,0), zeros };
        vertices[3] = { factor * vec4( 0.5, 0.5,0,1), sky, vec2(1,1), zeros };
        my_data.v_array = vertices;
        my_data.v.count = 4;

        std::vector<uint> quad_indices = {0,2,3,1};
        my_data.q_idx_array = new uint[4];
        std::copy(quad_indices.begin(), quad_indices.end(), my_data.q_idx_array);
        my_data.q_idx.count = 4;
        my_data.quad_count = 1;

        std::vector<uint> triangle_indices = {0,2,1,1,2,3};
        my_data.t_idx_array = new uint[6];
        std::copy(triangle_indices.begin(), triangle_indices.end(), my_data.t_idx_array);
        my_data.t_idx.count = 6;
        my_data.triangle_count = 2;
    }

    void loadOBJ(const string file_path){
        bool success = ParseObj(file_path, 0, my_data);
        if (!success){
            loadGrid();
            cerr << "Could not read file, loading grid" << endl;
        }
        if (my_data.quad_count > 0)
            quadtree->SetPrimType(QUADS);
        else if (my_data.triangle_count > 0)
            quadtree->SetPrimType(TRIANGLES);
        else
            cerr << "Could not map obj file" << endl;
    }

public:

    void Init(uint mode)
    {
        quadtree = new QuadTree();
        mode = mode;

        if(mode == TERRAIN)
            loadGrid();
        else if(mode == MESH)
            loadOBJ(obj_path);

        quadtree->Init(&my_data, INIT_SETTINGS);
    }

    void Draw(float deltaT, bool freeze)
    {
        if(!freeze &( mode != TERRAIN)){
            mat4 newM = glm::rotate(quadtree->model_mat, deltaT * 0.5f , vec3(0.0f, 0.0f, 1.0f));
            quadtree->SetModel(newM);
        }
        quadtree->Draw(deltaT);
    }

    void CleanUp()
    {
        quadtree->CleanUp();
        delete[] my_data.v_array;
        delete[] my_data.q_idx_array;
        delete[] my_data.t_idx_array;
    }

    void setMode(uint mode)
    {
        mode = mode;
        if (mode == TERRAIN){
            loadGrid();
            quadtree->settings.displace = true;
            quadtree->settings.adaptive_factor = 2.0;
            quadtree->Reinitialize();
        } else if (mode == MESH){
            loadOBJ(obj_path);
            quadtree->settings.displace = false;
            quadtree->settings.adaptive_factor = 0.7;
            quadtree->Reinitialize();
        }
        quadtree->SetModel(mat4(1.0));
    }
    QuadTree* GetQuadtreePointer()
    {
        return quadtree;
    }
};
#endif // MESH_H
