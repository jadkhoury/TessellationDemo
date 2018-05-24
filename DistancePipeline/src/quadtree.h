#ifndef QUADTREE_H
#define QUADTREE_H

#include <vector>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "glad/glad.h"
#include "utility.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace glm;
using namespace std;
using std::vector;

enum {TRIANGLES, QUADS};

struct DemoSettings
{
    bool uniform_tess = false;
    int uniformm_lvl = 0;
    float lod_factor = 0.7;
    bool morph = true;
    bool readback_primcount = false;
    bool freeze = false;
    bool displace = true;
    int cpu_lod = 2;
    int color_mode = 0;
    bool renderMVP = true;
};

struct Mesh_Data
{
    vec2* v_array;
    uint* idx_array;
    int poly_count, poly_type = TRIANGLES;
    Buffer v_buffer, idx_buffer;

};

struct Quadtree
{
    const uint TREE_MAX_LENGTH = 2048;
    const uint TREE_MAX_SIZE = TREE_MAX_LENGTH * sizeof(vec4);

    uint pingpong;
    uint tree_bo[3];
    bool mesh_initialized = false;
    DemoSettings settings;
    Mesh_Data& current_mesh;

    bool LoadNodesBuffers()
    {
        if (!mesh_initialized) {
            cout << "Load a mesh first" << endl;
            return false;
        }

        uvec4 nodes_array[TREE_MAX_SIZE];
        uint factor = (current_mesh.poly_type == TRIANGLES) ? 3 : 4;

        for (int i = 0; i < current_mesh.poly_count; ++i) {
            nodes_array[i] = uvec4(0, 0x1, uint(i*factor), 0);
        }

        utility::DeleteBuffer(&tree_bo[0]);
        utility::DeleteBuffer(&tree_bo[1]);
        utility::DeleteBuffer(&tree_bo[2]);

        glCreateBuffers(3, tree_bo);
        glNamedBufferStorage(tree_bo[0], TREE_MAX_SIZE, nodes_array, 0);
        glNamedBufferStorage(tree_bo[1], TREE_MAX_SIZE, nodes_array, 0);
        glNamedBufferStorage(tree_bo[2], TREE_MAX_SIZE, nodes_array, 0);
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

        current_mesh = {};
        current_mesh.v_buffer.count = vertices.size();
        current_mesh.v_buffer.size = current_mesh.v_buffer.count * sizeof(vec2);

        current_mesh.v_buffer.Reset();
        glNamedBufferStorage(current_meshv.bo, leaf_geometry.v.size, (const void*)vertices.data(), 0);

        leaf_geometry.idx.count = indices.size() * 3;
        leaf_geometry.idx.size =leaf_geometry.idx.count * sizeof(uint);
        utility::EmptyBuffer(&leaf_geometry.idx.bo);
        glCreateBuffers(1, &leaf_geometry.idx.bo);
        glNamedBufferStorage(leaf_geometry.idx.bo, leaf_geometry.idx.size, (const void*)indices.data(),  0);
        return (glGetError() == GL_NO_ERROR);
    }
};



#endif
