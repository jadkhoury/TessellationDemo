#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include "utility.h"
#include "quadtree.h"

// #define TINYOBJLOADER_IMPLEMENTATION
// #include "tiny_obj_loader.h"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec4;
std::vector<tinyobj::shape_t> shapes_;

enum {
    TERRAIN,
    MESH,
    NUM_MODES
};

using namespace std;
class Mesh
{
private:
    int num_grid_quads_;
    Mesh_Data my_data;
    QuadTree* quadtree_;
    uint mode_;
    QuadtreeSettings* qts;

    std::vector<tinyobj::shape_t> shapes_;


    int roundUpToSq(int n){
        int sq = ceil(sqrt(n));
        return (sq * sq);
    }


    void loadGrid()
    {
        const uint16_t* indices;
        int num_div = int(sqrt(num_grid_quads_)) - 1;

        djg_mesh* mesh = djgm_load_plane(1.0, 1.0, num_div, num_div);

        int num;
        // Vertices
        //        my_data.v_array = djgm_get_vertices(mesh, &num);
        const djgm_vertex* vertices = djgm_get_vertices(mesh, &num);
        Vertex* v_array_tmp = new Vertex[num];
        vec4 sky(0.0, 0.0, 1.0, 1.0);
        float factor = 5.0;
        for (int i = 0; i < num; ++i) {
            v_array_tmp[i].p = vec4(factor * vertices[i].p.x, factor * vertices[i].p.y, 0.0, 1.0);
            v_array_tmp[i].n = sky;
            v_array_tmp[i].uv = vec2(vertices[i].st.s, vertices[i].st.t);
        }
        my_data.v_array = v_array_tmp;
        my_data.v.count = num;

        // Quad indices
        indices = djgm_get_quads(mesh, &num);
        my_data.q_idx.count = num;
        my_data.q_idx_array = new uint[ my_data.q_idx.count];
        for (uint i = 0; i <  my_data.q_idx.count; ++i) {
            my_data.q_idx_array[i] = uint(indices[i]);
        }

        // Triangle indices
        indices = djgm_get_triangles(mesh, &num);
        my_data.t_idx.count = num;
        my_data.t_idx_array = new uint[my_data.t_idx.count];
        for (uint i = 0; i < my_data.t_idx.count; ++i) {
            my_data.t_idx_array[i] = uint(indices[i]);
        }
    }

    void loadOBJ(const string& filename){
        string error;

        vector<tinyobj::material_t> materials;

        bool objLoadReturn = tinyobj::LoadObj(shapes_, materials, error,
                                              filename.c_str());

        if(!error.empty()) {
            cerr << error << endl;
        }

        if(!objLoadReturn) {
            exit(EXIT_FAILURE);
        }

        my_data.v.count = shapes_[0].mesh.positions.size() / 3;
        int num_normals  = shapes_[0].mesh.normals.size() / 3;
        int num_uv =  shapes_[0].mesh.texcoords.size() / 3;


        my_data.t_idx.count  = shapes_[0].mesh.indices.size();



        Vertex* v_array_tmp = new Vertex[my_data.v.count];

        int idx = 0;
        for (uint i = 0; i < my_data.v.count; i++) {
            idx = i * 3;

            v_array_tmp[i].p = vec4(
                        shapes_[0].mesh.positions[idx+0],
                        shapes_[0].mesh.positions[idx+1],
                        shapes_[0].mesh.positions[idx+2],
                        1.0 );

            if(num_normals > 0)
                v_array_tmp[i].n = vec4(
                        shapes_[0].mesh.normals[idx+0],
                        shapes_[0].mesh.normals[idx+1],
                        shapes_[0].mesh.normals[idx+2],
                        0.0 );

            if(num_uv > 0)
                v_array_tmp[i].uv = vec2(
                            shapes_[0].mesh.texcoords[idx+0],
                            shapes_[0].mesh.texcoords[idx+1] );
        }

        my_data.v_array = v_array_tmp;

        my_data.t_idx_array = new uint[my_data.t_idx.count];
        for (uint i = 0; i < my_data.t_idx.count; ++i) {
            my_data.t_idx_array[i] = uint(shapes_[0].mesh.indices[i]);
        }



        printf("Loaded mesh '%s' (#V=%d, #I=%d, #N=%d)\n", filename.c_str(),
               my_data.v.count/3, my_data.t_idx.count, num_normals/3);

    }

public:

    QuadtreeSettings* Init(QuadTree* qt, uint mode)
    {
        num_grid_quads_ = 1;
        num_grid_quads_ = roundUpToSq(num_grid_quads_);
        quadtree_ = qt;
        mode_ = mode;

        if(mode == TERRAIN)
            loadGrid();
        else if(mode == MESH)
            loadOBJ("../../common/tangle_cube.obj");

        qts = quadtree_->Init(&my_data);
        if(mode == TERRAIN){
            qts->displace = true;
            qts->adaptive_factor = 0.0;
        }else if(mode == MESH){
            qts->displace = false;
            qts->adaptive_factor = 0.0;
        }
        quadtree_->ReconfigureShaders();

        return qts;

    }

    void Draw(float deltaT, bool freeze)
    {
        if(!freeze &( mode_ != TERRAIN)){
            mat4 newM = glm::rotate(quadtree_->GetModel(), deltaT * 0.5f , vec3(0.0f, 0.0f, 1.0f));
            quadtree_->SetModel(newM);
        }
        quadtree_->Draw(deltaT);
    }

    void CleanUp()
    {
        quadtree_->CleanUp();
        delete[] my_data.v_array;
        delete[]  my_data.q_idx_array;
        delete[] my_data.t_idx_array;
    }

    void setMode(uint mode)
    {
        mode_ = mode;
        if (mode_ == TERRAIN){
            loadGrid();
            qts->displace = true;
            qts->adaptive_factor = 2.0;
            quadtree_->Reinitialize();
        } else if (mode_ == MESH){
            loadOBJ("../../common/tangle_cube.obj");
            qts->displace = false;
            qts->adaptive_factor = 0.7;
            quadtree_->Reinitialize();
        }
        quadtree_->SetModel(mat4(1.0));

    }
};
#endif // MESH_H
