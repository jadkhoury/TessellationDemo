#line 1001

#ifndef LTREE_GLSL
#define LTREE_GLSL

#define M_PI 3.1415926535897932384626433832795
#define SQRT_2_INV 0.70710678118;

#define X_AXIS 0
#define Y_AXIS 1

#define X_STATE   0
#define Y_STATE   1
#define INC_STATE 2
#define DEC_STATE 3



// ************************ DECLARATIONS ************************ //

// Structures
struct Vertex {
    vec4 p;
    vec4 n;
    vec2 uv;
    vec2 align;
};


struct Triangle {
    Vertex vertex[3];
};

struct Quad {
    Vertex vertex[4];
};

struct Triangle2D {
    vec2 p[3]; // vertices
    vec2 uv[3]; // texcoords
};

// Buffers
layout (std430, binding = NODES_IN_B) readonly buffer Data_In
{
    uvec4 nodes_in[];
};

layout (std430, binding = NODES_OUT_FULL_B) buffer Data_Out_F
{
    uvec4 nodes_out_full[];
};

layout (std430, binding = NODES_OUT_CULLED_B) buffer Data_Out_C
{
    uvec4 nodes_out_culled[];
};

layout (std430, binding = MESH_V_B) readonly buffer Mesh_V
{
    Vertex mesh_v[];
};
layout (std430, binding = MESH_Q_IDX_B) readonly buffer Mesh_Q_Idx
{
    uint mesh_q_idx[];
};
layout (std430, binding = MESH_T_IDX_B) readonly buffer Mesh_T_Idx
{
    uint  mesh_t_idx[];
};

uvec4 lt_getKey_64(uint idx)
{
    return nodes_in[idx];
}

// --------- Bitwise operations reimplemented for concatenated ints --------- //

uvec2 lt_leftShift_64(uvec2 nodeID, uint shift)
{
    uvec2 result = nodeID;
    //Extract the "shift" first bits of y and append them at the end of x
    result.x = result.x << shift;
    result.x |= result.y >> (32 - shift);
    result.y  = result.y << shift;
    return result;
}
uvec2 lt_rightShift_64(uvec2 nodeID, uint shift)
{
    uvec2 result = nodeID;
    //Extract the "shift" last bits of x and prepend them to y
    result.y = result.y >> shift;
    result.y |= result.x << (32 - shift);
    result.x = result.x >> shift;
    return result;
}

int lt_findMSB_64(uvec2 nodeID)
{
    return (nodeID.x == 0) ? findMSB(nodeID.y) : (findMSB(nodeID.x) + 32);
}

// -------------------------- Children and Parents -------------------------- //

void lt_children_64 (uvec2 nodeID, out uvec2 children[4])
{
    nodeID = lt_leftShift_64(nodeID, 2u);
    children[0] = nodeID;
    children[1] = uvec2(nodeID.x, nodeID.y | 0x1);
    children[2] = uvec2(nodeID.x, nodeID.y | 0x2);
    children[3] = uvec2(nodeID.x, nodeID.y | 0x3);
}

uvec2 lt_parent_64 (uvec2 nodeID)
{
    return lt_rightShift_64(nodeID, 2u);
}

// --------------------------------- Level ---------------------------------- //

uint lt_level_64 (uvec2 nodeID)
{
    uint i = lt_findMSB_64(nodeID);
    return (i >> 1u);
}

// ------------------------------ Leaf & Root ------------------------------- //

bool lt_isLeaf_64 (uvec2 nodeID)
{
    return (lt_level_64(nodeID) == 31u);
}

bool lt_isRoot_64 (uvec2 nodeID)
{
    return (lt_findMSB_64(nodeID) == 0u);
}

// -------------------------------- Position -------------------------------- //

bool lt_isLeft_64 (uvec2 nodeID)
{
    return !bool(nodeID.y & 0x1);
}

bool lt_isRight_64 (uvec2 nodeID)
{
    return bool(nodeID.y & 0x1);
}

bool lt_isUpper_64 (uvec2 nodeID)
{
    return !bool(nodeID.y & 0x2);
}

bool lt_isLower_64 (uvec2 nodeID)
{
    return bool(nodeID.y & 0x2);
}

bool lt_isUpperLeft_64 (uvec2 nodeID)
{
    return !(bool(nodeID.y & 0x3));
}


// ----------------------------- Triangle XForm ----------------------------- //

//#define OLD
#ifdef OLD
mat3 jk_bitToMatrix(uint b)
{
    float bf = b;
    float s = 2.0 * bf - 1.0;
    return mat3(+s*0.5,   -0.5, 0.0,
                  -0.5, -s*0.5, 0.0,
                   0.5,    0.5, 1.0);
}

void lt_getTriangleXform_64 (uvec2 nodeID, out mat3 xform, out mat3 parent_xform)
{
    mat3 xf = mat3(1);

    // Handles the root triangle case
    if (nodeID.x == 0 && nodeID.y == 1){
        xform = xf;
        return;
    }

    uint lsb = nodeID.y & 3;
    nodeID = lt_rightShift_64(nodeID, 2);
    while (nodeID.x > 0 || nodeID.y > 1) {
        xf = jk_bitToMatrix(nodeID.y & 1) * xf;
        nodeID = lt_rightShift_64(nodeID, 1);
    }

    parent_xform = xf;
    xform = xf * jk_bitToMatrix((lsb >> 1u) & 1u) * jk_bitToMatrix(lsb & 1u);
}


void lt_getTriangleXform_64 (uvec2 nodeID, out mat3 xform)
{
    mat3 tmp;
    lt_getTriangleXform_64(nodeID, xform, tmp);
}


#else

mat3x2 mul(mat3x2 A, mat3x2 B)
{
    mat2 tmp = mat2(A) * mat2(B);
    mat3x2 r = mat3x2(tmp);
    mat2x3 T = transpose(A);
    r[2].x = dot(T[0], vec3(B[2], 1.0));
    r[2].y = dot(T[1], vec3(B[2], 1.0));
    return r;
}

mat3x2 jk_bitToMatrix(in uint bit)
{
    float s = float(bit) - 0.5;
    vec2 c1 = vec2(+s, -0.5);
    vec2 c2 = vec2(-0.5, -s);
    vec2 c3 = vec2(+0.5, +0.5);
    return mat3x2(c1, c2, c3);
}

void lt_getTriangleXform_64 (uvec2 nodeID, out mat3x2 xform, out mat3x2 parent_xform)
{
    vec2 c1 = vec2(1, 0);
    vec2 c2 = vec2(0, 1);
    vec2 c3 = vec2(0, 0);
    mat3x2 xf = mat3x2(c1, c2, c3);

    // Handles the root triangle case
    if (nodeID.x == 0 && nodeID.y == 1){
        xform = xf;
        return;
    }

    uint lsb = nodeID.y & 3;
    nodeID = lt_rightShift_64(nodeID, 2);
    while (nodeID.x > 0 || nodeID.y > 1) {
        xf = mul(jk_bitToMatrix(nodeID.y & 1) , xf);
        nodeID = lt_rightShift_64(nodeID, 1);
    }

    parent_xform = xf;
    xform = mul(xf, jk_bitToMatrix((lsb >> 1u) & 1u));
    xform = mul(xform, jk_bitToMatrix(lsb & 1u));
}


void lt_getTriangleXform_64 (uvec2 nodeID, out mat3x2 xform)
{
    mat3x2 tmp;
    lt_getTriangleXform_64(nodeID, xform, tmp);
}

#endif
// --------------------------- Mapping to polygon  --------------------------- //

// *** Map to given primitive *** //
vec2 lt_mapTo2DTriangle(Triangle2D t, vec2 uv)
{
    return (1.0 - uv.x - uv.y) * t.p[0] + uv.x * t.p[2] + uv.y * t.p[1];
}

vec4 lt_mapTo3DTriangle(Triangle t, vec2 uv)
{
    vec4 result = (1.0 - uv.x - uv.y) * t.vertex[0].p +
            uv.x * t.vertex[2].p +
            uv.y * t.vertex[1].p;
    return result;
}

vec4 lt_mapTo3DQuad(Quad q, vec2 uv)
{
    vec4 p01 = mix(q.vertex[0].p, q.vertex[1].p, uv.x);
    vec4 p32 = mix(q.vertex[3].p, q.vertex[2].p, uv.x);
    return mix(p01, p32, uv.y);
}

// ------------------------- Fetching Mesh Polygon  ------------------------- //
void lt_getMeshTriangle(uint meshPolygonID, out Triangle triangle)
{
    for (int i = 0; i < 3; ++i)
    {
        triangle.vertex[i].p = mesh_v[mesh_t_idx[meshPolygonID + i]].p;
        triangle.vertex[i].n = mesh_v[mesh_t_idx[meshPolygonID + i]].n;
        triangle.vertex[i].uv = mesh_v[mesh_t_idx[meshPolygonID + i]].uv;
    }
}

void lt_getMeshQuad(uint meshPolygonID, out Quad quad)
{
    for (int i = 0; i < 4; ++i)
    {
        quad.vertex[i].p = mesh_v[mesh_q_idx[meshPolygonID + i]].p;
        quad.vertex[i].n = mesh_v[mesh_q_idx[meshPolygonID + i]].n;
        quad.vertex[i].uv = mesh_v[mesh_q_idx[meshPolygonID + i]].uv;
    }
}

void lt_getQuadMeshTriangle(uint meshPolygonID, uint rootID, out Triangle mesh_t)
{
    Quad q;
    lt_getMeshQuad(meshPolygonID, q);
    if (rootID == 0) {
        mesh_t.vertex[0] = q.vertex[0];
        mesh_t.vertex[1] = q.vertex[3];
        mesh_t.vertex[2] = q.vertex[1];
    } else {
        mesh_t.vertex[0] = q.vertex[2];
        mesh_t.vertex[1] = q.vertex[1];
        mesh_t.vertex[2] = q.vertex[3];
    }
}

vec4 lt_getMeanPrimNormal(uvec4 key, in int poly_type) {
    uint meshPolygonID = key.z;
    vec4 sum = vec4(0);

    if (poly_type == TRIANGLES)
        for (int i = 0; i < 3; ++i)
            sum += mesh_v[mesh_t_idx[meshPolygonID + i]].n;

    else if (poly_type == QUADS)
        for (int i = 0; i < 4; ++i)
            sum += mesh_v[mesh_q_idx[meshPolygonID + i]].n;

    sum = normalize(sum);
    return (sum == vec4(0)) ? vec4(0,1,0,0) : sum;
}

// ------------------------------ Node Mapping  ----------------------------- //
vec2 lt_Leaf_to_Tree_64(vec2 p, uvec2 nodeID, in bool parent)
{
#ifdef OLD
    mat3 xf, pxf, xform;
#else
    mat3x2 xf, pxf, xform;
#endif
    lt_getTriangleXform_64(nodeID, xf, pxf);
    xform = (parent) ? pxf : xf;
    return (xform * vec3(p, 1)).xy;
}

vec4 lt_Tree_to_MeshTriangle(vec2 p, uint poly_type, uint meshPolygonID, uint rootID)
{
    Triangle mesh_t;
    if (poly_type == TRIANGLES)
        lt_getMeshTriangle(meshPolygonID, mesh_t);
    else
        lt_getQuadMeshTriangle(meshPolygonID, rootID, mesh_t);
    return lt_mapTo3DTriangle(mesh_t, p);
}

vec4 lt_Tree_to_MeshTriangle(vec2 p, uint meshPolygonID)
{
    Triangle mesh_t;
    lt_getMeshTriangle(meshPolygonID, mesh_t);
    return lt_mapTo3DTriangle(mesh_t, p);
}

vec4 lt_Leaf_to_MeshPrimitive(vec2 p, uvec4 key, in bool parent, int poly_type)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    vec2 tmp = p;
    tmp = lt_Leaf_to_Tree_64(tmp, nodeID, parent);
    return lt_Tree_to_MeshTriangle(tmp, poly_type, meshPolygonID, rootID);
}

void lt_Leaf_n_Parent_to_MeshPrimitive(vec2 p, uvec4 key, out vec4 p_mesh, out vec4 pp_mesh,
                                       uint poly_type)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
#ifdef OLD
    mat3 xf, pxf;
#else
    mat3x2 xf, pxf;
#endif
    vec2 p2D, pp2D;

    lt_getTriangleXform_64(nodeID, xf, pxf);
    p2D = (xf * vec3(p, 1)).xy;
    pp2D = (pxf * vec3(p, 1)).xy;

    p_mesh  = lt_Tree_to_MeshTriangle(p2D, poly_type, meshPolygonID, rootID);
    pp_mesh = lt_Tree_to_MeshTriangle(pp2D, poly_type, meshPolygonID, rootID);
}

#endif
