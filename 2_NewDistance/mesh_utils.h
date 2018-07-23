#ifndef MESHUTILS_H
#define MESHUTILS_H

#include "common.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <fstream>
#include <sstream>


////////////////////////////////////////////////////////////////////////////////
///
/// namespace for generating and managing meshes
///

namespace meshutils {

// Loads a grid with the number of quads specified in grid_quads_count (rounded down to square)
// Stores the resulting mesh in mesh_data
void LoadGrid(Mesh_Data* mesh_data, int grid_quads_count)
{

    float factor = 10.0;

    const uint16_t* indices;
    int num_div = int(sqrt(grid_quads_count)) - 1;
    num_div = int(factor)- 1;
    djg_mesh* mesh = djgm_load_plane(num_div, num_div);

    int count;
    // Vertices
    const djgm_vertex* vertices = djgm_get_vertices(mesh, &count);
    Vertex* v_array_tmp = new Vertex[count];
    vec4 sky(0.0, 0.0, 1.0, 0.0);
    for (int i = 0; i < count; ++i) {
        v_array_tmp[i].p = vec4(factor * (vertices[i].p.x - 0.5),
                                factor * (vertices[i].p.y - 0.5),
                                0.0,
                                1.0);
        cout << glm::to_string(v_array_tmp[i].p) << endl;
        v_array_tmp[i].n = sky;
        v_array_tmp[i].uv = vec2(vertices[i].st.s, vertices[i].st.t);
    }
    mesh_data->v_array = v_array_tmp;
    mesh_data->v.count = count;

    // Quad indices
    indices = djgm_get_quads(mesh, &count);
    mesh_data->q_idx.count = count;
    mesh_data->q_idx_array = new uint[ mesh_data->q_idx.count];
    for (uint i = 0; i <  mesh_data->q_idx.count; ++i) {
        mesh_data->q_idx_array[i] = uint(indices[i]);
    }

    // Triangle indices
    indices = djgm_get_triangles(mesh, &count);
    mesh_data->t_idx.count = count;
    mesh_data->t_idx_array = new uint[mesh_data->t_idx.count];
    for (uint i = 0; i < mesh_data->t_idx.count; ++i) {
        int k = mesh_data->t_idx.count - i - 1;
        mesh_data->t_idx_array[i] = uint(indices[k]);
    }
    mesh_data->quad_count = mesh_data->q_idx.count / 4;
    mesh_data->triangle_count = mesh_data->t_idx.count / 3;
}

// Utility function to read from file
static char const * sgets( char * s, int size, char ** stream ) {
    for (int i=0; i<size; ++i) {
        if ( (*stream)[i]=='\n' || (*stream)[i]=='\0') {

            memcpy(s, *stream, i);
            s[i]='\0';

            if ((*stream)[i]=='\0')
                return 0;
            else {
                (*stream) += i+1;
                return s;
            }
        }
    }
    return 0;
}

// Read a .obj from file and store it in mesh_data
// Reads the data line by line
// Creates unique Vertex objects for each unencountered set of position / normal / UV
// Stores the result in mesh_data
// File parsing taken from OpenSubdiv
void ParseObj(string name, int axis, Mesh_Data* mesh_data)
{
    // Opening file and stuff
    ifstream instream(name);
    string contents((istreambuf_iterator<char>(instream)), istreambuf_iterator<char>());

    char const* shapestr = contents.c_str();

    char* str = const_cast<char *>(shapestr), line[256];
    bool done = false;

    // Filling independent vectors from the data
    vector<vec4> verts;
    vector<vec2> uvs;
    vector<vec4> normals;
    vector<int> faceverts;
    vector<int> faceuvs;
    vector<int> facenormals;
    vec3 p, n, max = vec3(0);
    int nvertsPerFace = -1;
    while (! done) {
        done = sgets(line, sizeof(line), &str)==0;
        char* end = &line[strlen(line)-1];
        if (*end == '\n') *end = '\0'; // strip trailing nl
        float x, y, z, u, v;
        switch (line[0]) {
        case 'v':
            switch (line[1]) {
            case ' ':
                if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3) {
                    p = (axis == 0) ? vec3(x, -z, y) : vec3(x,y,z);
                    max = glm::max(max, glm::abs(p));
                    verts.push_back(vec4(p, 1.0));
                } break;
            case 't':
                if (sscanf(line, "vt %f %f", &u, &v) == 2) {
                    uvs.push_back(vec2(u,v));
                } break;
            case 'n' :
                if (sscanf(line, "vn %f %f %f", &x, &y, &z) == 3) {
                    n = (axis == 0) ? vec3(x, -z, y) : vec3(x,y,z);
                    normals.push_back(vec4(glm::normalize(n), 0.0));
                } break;
            } break;
        case 'f':
            if (line[1] == ' ') {
                int vi = -1, ti = -1, ni = -1;
                const char* cp = &line[2];
                while (*cp == ' ') cp++;
                int nverts = 0, nitems=0;
                while( (nitems=sscanf(cp, "%d/%d/%d", &vi, &ti, &ni))>0) {
                    nverts++;
                    faceverts.push_back(vi-1);
                    if(ti > 0) faceuvs.push_back(ti-1);
                    if(ni > 0) facenormals.push_back(ni-1);
                    while (*cp && *cp != ' ') cp++;
                    while (*cp == ' ') cp++;
                }
                if (nvertsPerFace == -1){
                    nvertsPerFace = nverts;
                }
                if (nverts != 3 && nverts != 4)
                    throw runtime_error("Need quads or triangle faces");
                if (nverts != nvertsPerFace)
                    throw runtime_error("Need faces with same number of vertices");
            } break;
        default:
            break;
        }
    }

    float m = glm::compMax(max);
    for (int i = 0; i < (int)verts.size(); ++i) {
        verts[i] /= vec4(m,m,m,1);
    }

    // Putting this data in well constructed data structure
    vector<Vertex> vert_vector;
    vector<uint> idx_vector;
    map<uint, uint> uniqueIDX_to_vectorIDX;
    Vertex current_v;
    int num_idx = faceverts.size();
    int Nv = verts.size();
    int Nn = normals.size();
    int Nt = uvs.size();
    cout << "Mesh Data:" << endl;
    cout << Nv << " Vertice, " << endl;
    cout << Nn << " Normals, " << endl;
    cout << Nt << " UVs" << endl;
    int iv = 0, it = 0, in = 0;
    uint64 unique_idx;
    bool with_normals = facenormals.size() > 0;
    bool with_uvs = faceuvs.size() > 0;

    for (int i = 0;  i < num_idx; i++)
    {
        iv = faceverts[i];
        if (with_normals) in = facenormals[i];
        if (with_uvs)  it = faceuvs[i];
        unique_idx = iv + Nv * (in + Nn * it);
        if (uniqueIDX_to_vectorIDX.count(unique_idx) == 0) {
            current_v.p = verts[iv];
            if (with_normals)  current_v.n = normals[in];
            if (with_uvs) current_v.uv = uvs[it];
            uniqueIDX_to_vectorIDX[unique_idx] = vert_vector.size();
            vert_vector.push_back(current_v);
             // cout << "Created vertex " << unique_idx << " at idx " << uniqueIDX_to_vectorIDX[unique_idx] << endl;
             // cout << current_v.to_string() << endl;
        } else {
             // cout << "Vertex at " << unique_idx << " already created at  "<< uniqueIDX_to_vectorIDX[unique_idx]  << endl;
             // cout << vert_vector[uniqueIDX_to_vectorIDX[unique_idx]].to_string() << endl;
        }
        idx_vector.push_back(uniqueIDX_to_vectorIDX[unique_idx]);
    }
    cout << vert_vector.size() << " unique vertices created" << endl;
    cout << faceverts.size() << " faceverts" << endl;
    cout << facenormals.size() << " facenormals" << endl;

    //Putting data in Mesh_data structure
    if (nvertsPerFace == 3) {
        mesh_data->t_idx_array = new uint[idx_vector.size()];
        std::copy(idx_vector.begin(), idx_vector.end(), mesh_data->t_idx_array);
        mesh_data->t_idx.count = idx_vector.size();
        mesh_data->q_idx.count = 0;
    } else {
        mesh_data->q_idx_array = new uint[idx_vector.size()];
        std::copy(idx_vector.begin(), idx_vector.end(), mesh_data->q_idx_array);
        mesh_data->q_idx.count = idx_vector.size();
        mesh_data->t_idx.count = 0;
    }
    mesh_data->v_array = new Vertex[vert_vector.size()];
    std::copy(vert_vector.begin(), vert_vector.end(), mesh_data->v_array);
    mesh_data->v.count = vert_vector.size();
    mesh_data->triangle_count = mesh_data->t_idx.count / 3;
    mesh_data->quad_count = mesh_data->q_idx.count / 4;

    // Deleting old data structures
    verts.clear();
    uvs.clear();
    normals.clear();
    faceverts.clear();
    faceuvs.clear();
    facenormals.clear();
    idx_vector.clear();
    vert_vector.clear();
}

}

#endif
