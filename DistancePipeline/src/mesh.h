#ifndef MESH_H
#define MESH_H

//#include <iterator>
#include "utility.h"
#include "quadtree.h"

#include <fstream>
#include <glm/gtx/component_wise.hpp>

//#include "obj_loader.h"

// #define TINYOBJLOADER_IMPLEMENTATION
// #include "tiny_obj_loader.h"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::uvec4;
std::vector<tinyobj::shape_t> shapes_;

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
        Vertex* vertices = new Vertex[4];
        vec4 sky(0.0, 0.0, 1.0, 1.0);
        float factor = 5.0;
        vec2 zeros(0,0);

        vertices[0] = { factor * vec4(0,0,0,1), sky, vec2(0,0), zeros };
        vertices[1] = { factor * vec4(0,1,0,1), sky, vec2(0,1), zeros };
        vertices[2] = { factor * vec4(1,0,0,1), sky, vec2(1,0), zeros };
        vertices[3] = { factor * vec4(1,1,0,1), sky, vec2(1,1), zeros };
        my_data.v_array = vertices;
        my_data.v.count = 4;

        uint quad_indices[4] = {0,2,3,1};
        my_data.q_idx.count = 4;
        my_data.q_idx_array = new uint[4];
        my_data.quad_count = 1;
        for (int i  = 0; i < 4; ++i) {
            my_data.q_idx_array[i] = quad_indices[i];
        }

        uint triangle_indices[6] = {0,2,1,1,2,3};
        my_data.t_idx.count = 6;
        my_data.triangle_count = 2;
        my_data.t_idx_array = new uint[6];
        for (int i  = 0; i < 6; ++i) {
            my_data.t_idx_array[i] = triangle_indices[i];
        }
    }

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

    bool ParseObj(string file_path, int axis, Mesh_Data* mesh_data)
    {
        // Opening file and stuff
        ifstream instream(file_path);
        string contents((istreambuf_iterator<char>(instream)), istreambuf_iterator<char>());
        if (contents.empty()){
            cerr << "ERROR in ParseOBJ: Could not read OBJ" << endl;
            return false;
        }

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
        for (int i = 0; i < verts.size(); ++i) {
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
        ulong unique_idx;
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
        return true;
    }

    void loadOBJ(const string file_path){
        bool success = ParseObj(file_path, 0, &my_data);
//        if (!success)
//            loadGrid();
    }

public:

    QuadtreeSettings* Init(QuadTree* qt, uint mode)
    {

        quadtree_ = qt;
        mode_ = mode;
        QuadtreeSettings qtest = qt->qts;



        if(mode == TERRAIN)
            loadGrid();
        else if(mode == MESH)
            loadOBJ("../src/objs/bigguy.obj");

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
            loadOBJ("../src/objs/bigguy.obj");
            qts->displace = false;
            qts->adaptive_factor = 0.7;
            quadtree_->Reinitialize();
        }
        quadtree_->SetModel(mat4(1.0));

    }
};
#endif // MESH_H
