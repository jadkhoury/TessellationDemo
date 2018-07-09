#ifndef MESH_H
#define MESH_H

#include "quadtree.h"
#include "common.h"

class Mesh
{
public:
    QuadTree* quadtree;
    TransformsManager* tranforms_manager;
    QuadTree::Settings init_settings;

    Mesh_Data mesh_data;
    uint grid_quads_count;

    int roundUpToSq(int n)
    {
        int sq = ceil(sqrt(n));
        return (sq * sq);
    }

    /*
     * Reorder the triangle indices of a mesh such that the point 0 of each
     * triangle faces the hypotenuse
     */
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

            max_d = std::max(d01, std::max(d02, d12));
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


    /*
     * Fill the Mesh_Data structure with vertices and indices
     * Either:
     * - Loads a grid for the terrain mode
     * - Loads an .obj file using the parsing function in mesh_utils.h
     * Depending on the index array filled by the parsing function, sets the
     * quadtree to the correct polygon rendering mode
     */
    void LoadMeshData(string filepath)
    {
        if (filepath == "") {
            meshutils::LoadGrid(&mesh_data,  grid_quads_count);
            LoadMeshBuffers();
            init_settings.displace = true;
            init_settings.adaptive_factor = 50.0;

        } else {
            meshutils::ParseObj(filepath, 0, &mesh_data);
            if (mesh_data.quad_count > 0 && mesh_data.triangle_count == 0) {
                init_settings.poly_type = QUADS;
            } else if (mesh_data.quad_count == 0 && mesh_data.triangle_count > 0) {
                init_settings.poly_type = TRIANGLES;
            } else {
                cout << "ERROR when parsing obj" << endl;
            }
            init_settings.adaptive_factor = 1.0;
            LoadMeshBuffers();
            init_settings.displace = false;
        }
    }

    /*
     * Loads the data in the Mesh_Data structure into buffers.
     * Creates:
     * - 1 buffer for (unique) vertices
     * - 1 buffer for quad indices
     * - 1 buffer for triangle indices
     * It seemed easier to have for index buffer available on the gpu and switch
     * from one to the other depending on the mesh polygon used
     */
    bool LoadMeshBuffers()
    {
        utility::EmptyBuffer(&mesh_data.v.bo);
        glCreateBuffers(1, &(mesh_data.v.bo));
        glNamedBufferStorage(mesh_data.v.bo,
                             sizeof(Vertex) * mesh_data.v.count,
                             (const void*)(mesh_data.v_array),
                             0);


        utility::EmptyBuffer(&mesh_data.q_idx.bo);
        if (mesh_data.quad_count > 0) {
            glCreateBuffers(1, &(mesh_data.q_idx.bo));
            glNamedBufferStorage(mesh_data.q_idx.bo,
                                 sizeof(uint) * mesh_data.q_idx.count,
                                 (const void*)(mesh_data.q_idx_array),
                                 0);
        }

        reorderIndices(mesh_data.t_idx_array, mesh_data.v_array, mesh_data.t_idx.count);
        utility::EmptyBuffer(&mesh_data.t_idx.bo);
        glCreateBuffers(1, &(mesh_data.t_idx.bo));
        if (mesh_data.triangle_count > 0) {
            glNamedBufferStorage(mesh_data.t_idx.bo,
                                 sizeof(uint) * mesh_data.t_idx.count,
                                 (const void*)(mesh_data.t_idx_array),
                                 0);
        }

        cout << "Mesh has " << mesh_data.quad_count << " quads, "
             << mesh_data.triangle_count << " triangles " << endl;

        return (glGetError() == GL_NO_ERROR);
    }

    void UpdateTransforms()
    {
        tranforms_manager->UpdateMV();
    }

    void Init(string filepath)
    {
        quadtree = new QuadTree();
        tranforms_manager = new TransformsManager();
        mesh_data = {};
        grid_quads_count = 5;
        grid_quads_count = roundUpToSq(grid_quads_count);

        init_settings = {
            /*int uni_lvl*/ 0,
            /*float adaptive_factor*/ 1,
            /*bool uniform*/ true,
            /*bool map_primcount*/ true,
            /*bool rotateMesh*/ false,
            /*bool displace*/ true,
            /*int color_mode*/ LOD,
            /*bool render_projection*/ true,

            /*int poly_type*/ TRIANGLES,
            /*bool morph*/ true,
            /*bool freeze*/ false,
            /*int cpu_lod*/ 2,
            /*bool cull*/ true,
            /*bool debug_morph*/ false,
            /*float morph_k*/ 0.0,
            /*uint wg_count*/ 512
        };

        this->LoadMeshData(filepath);
        quadtree->Init(&(this->mesh_data), this->tranforms_manager, init_settings);
    }

    void Draw(float deltaT, uint mode)
    {
        if (!quadtree->settings.freeze && (mode == MESH) &&  quadtree->settings.rotateMesh) {
            tranforms_manager->transforms.M = glm::rotate(tranforms_manager->transforms.M, 2.0f*deltaT , vec3(0.0f, 0.0f, 1.0f));
            UpdateTransforms();
        }
        tranforms_manager->UploadTransforms();
        quadtree->Draw(deltaT);
    }

};


#endif  //MESH_H
