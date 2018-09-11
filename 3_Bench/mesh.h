#ifndef MESH_H
#define MESH_H

#include "quadtree.h"
#include "common.h"
#include "transform.h"

class Mesh
{
public:
    QuadTree* quadtree;
    TransformsManager* tranforms_manager;
    QuadTree::Settings init_settings;

    Mesh_Data mesh_data;

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
    void LoadMeshData(uint mode, string filepath = "")
    {
        if (mode == TERRAIN) {
            meshutils::LoadGrid(&mesh_data);
            LoadMeshBuffers();
        } else if (mode == MESH){
            meshutils::ParseObj(filepath, 0, &mesh_data);
            if (mesh_data.quad_count > 0 && mesh_data.triangle_count == 0) {
                init_settings.polygon_type = QUADS;
            } else if (mesh_data.quad_count == 0 && mesh_data.triangle_count > 0) {
                init_settings.polygon_type = TRIANGLES;
            } else {
                cout << "ERROR when parsing obj" << endl;
            }
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


    // ----- Transform call passthrough ---- //
    void UpdateForFOV(CameraManager& cam) {
        tranforms_manager->UpdateForNewFOV(cam);
    }

    void UpdateForSize(CameraManager& cam) {
        tranforms_manager->UpdateForNewSize(cam);
    }

    void UpdateForView(CameraManager& cam) {
        tranforms_manager->UpdateForNewView(cam);
    }

    void InitTransforms(CameraManager& cam) {
        tranforms_manager->Init(cam);
    }

    void Init(uint mode, CameraManager& cam, string filepath = "")
    {
        quadtree = new QuadTree();
        tranforms_manager = new TransformsManager();
        mesh_data = {};

        init_settings.uniform_on = false;
        init_settings.uniform_lvl = 0;
        init_settings.lod_factor = 1;
        init_settings.target_e_length = 10;
        init_settings.map_nodecount = false;
        init_settings.rotateMesh = (mode == MESH);
        init_settings.displace_on = false;
        init_settings.displace_factor = 0.3f;
        init_settings.color_mode = LOD;

        init_settings.projection_on = true;
        init_settings.wireframe_on = true;
        init_settings.flat_normal = true;


        init_settings.polygon_type = TRIANGLES;
        init_settings.freeze = false;
        init_settings.cpu_lod = 2;
        init_settings.cull_on = true;

        init_settings.itpl_type = PHONG;
        init_settings.itpl_alpha = 1;

        this->LoadMeshData(mode, filepath);
        this->LoadMeshBuffers();

        this->tranforms_manager->Init(cam);
        quadtree->Init(&(this->mesh_data), this->tranforms_manager->GetBo(), init_settings);
    }

    void Draw(float deltaT, uint mode)
    {
        if (!quadtree->settings.freeze &&  quadtree->settings.rotateMesh) {
            tranforms_manager->RotateModel(2.0f * deltaT, vec3(0.0f, 0.0f, 1.0f));
        }
        tranforms_manager->Upload();
        quadtree->Draw(deltaT);
    }

    void CleanUp()
    {
        quadtree->CleanUp();
        tranforms_manager->CleanUp();
        mesh_data.CleanUp();
    }

};


#endif  //MESH_H
