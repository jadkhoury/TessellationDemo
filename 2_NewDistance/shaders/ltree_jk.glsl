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

uvec4 lt_getKey_64(uint idx);

uvec2 lt_leftShift_64(in uvec2 nodeID, in uint shift);
uvec2 lt_rightShift_64(uvec2 nodeID, uint shift);
int lt_findMSB_64(uvec2 nodeID);

void lt_children_64 (in uvec2 nodeID, out uvec2 children[4]);
uvec2 lt_parent_64 (in uvec2 nodeID);
uint lt_level_64 (in uvec2 nodeID);

bool lt_isLeaf_64 (in uvec2 nodeID);
bool lt_isRoot_64 (in uvec2 nodeID);
bool lt_isLeft_64 (in uvec2 nodeID);
bool lt_isRight_64 (in uvec2 nodeID);
bool lt_isUpper_64 (in uvec2 nodeID);
bool lt_isLower_64 (in uvec2 nodeID);
bool lt_isUpperLeft_64 (in uvec2 nodeID);

void lt_getTriangleXform_64 (in uvec2 key, out mat3 xform, out mat3 parent_xform);
void lt_getTriangleXform_64 (in uvec2 nodeID, out mat3 xform);

vec2 lt_mapTo2DTriangle(Triangle2D t, vec2 uv);
vec4 lt_mapTo3DTriangle(Triangle t, vec2 uv);
vec4 lt_mapTo3DQuad(Quad q, vec2 uv);

void lt_getMeshTriangle(in uint meshPolygonID, out Triangle triangle);
void lt_getMeshQuad(in uint meshPolygonID, out Quad quad);
void lt_getRootTriangle(in uint rootID, out Triangle2D t);
vec4 lt_getMeanPrimNormal(in uvec4 key, in int poly_type);

vec2 lt_Leaf_to_Tree_64(in vec2 p, in uvec2 nodeID, in bool parent);
vec2 lt_Leaf_to_Tree_64(in vec2 p, uvec2 nodeID);

vec4 lt_Tree_to_MeshTriangle(in vec2 p, in uint meshPolygonID);

vec2 lt_Tree_to_QuadRoot(in vec2 p, in uint rootID);
vec4 lt_Root_to_MeshQuad(in vec2 p, in uint meshPolygonID);

vec4 lt_Leaf_to_MeshTriangle(in vec2 p, in uvec4 key, in bool parent);
vec4 lt_Leaf_to_MeshQuad(in vec2 p, in uvec4 key, in bool parent);
vec4 lt_Leaf_to_MeshPrimitive(in vec2 p, in uvec4 key, in bool parent, int poly_type);

void lt_Leaf_n_Parent_to_MeshTriangle(in vec2 p, in uvec4 key, out vec4 p_mesh, out vec4 pp_mesh);
void lt_Leaf_n_Parent_to_MeshQuad(in vec2 p, in uvec4 key, out vec4 p_mesh, out vec4 pp_mesh);
void lt_Leaf_n_Parent_to_MeshPrimitive(in vec2 p, in uvec4 key,
                                       out vec4 p_mesh,
                                       out vec4 pp_mesh,
                                       uint poly_type);

vec4 lt_Tree_to_MeshTriangle(in vec2 p, in uvec4 key, in bool parent);
vec4 lt_Tree_to_MeshQuad(in vec2 p, in uvec4 key, in bool parent);
vec4 lt_Tree_to_MeshPrimitive(in vec2 p, in uvec4 key, in bool parent, int poly_type);

uvec4 lt_getKey_64(uint idx)
{
    return nodes_in[idx];
}

// --------- Bitwise operations reimplemented for concatenated ints --------- //

uvec2 lt_leftShift_64(in uvec2 nodeID, in uint shift)
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

void lt_children_64 (in uvec2 nodeID, out uvec2 children[4])
{
    nodeID = lt_leftShift_64(nodeID, 2u);
    children[0] = nodeID;
    children[1] = uvec2(nodeID.x, nodeID.y | 0x1);
    children[2] = uvec2(nodeID.x, nodeID.y | 0x2);
    children[3] = uvec2(nodeID.x, nodeID.y | 0x3);
}

uvec2 lt_parent_64 (in uvec2 nodeID)
{
    return lt_rightShift_64(nodeID, 2u);
}

// --------------------------------- Level ---------------------------------- //

uint lt_level_64 (in uvec2 nodeID)
{
    uint i = lt_findMSB_64(nodeID);
    return (i >> 1u);
}

// ------------------------------ Leaf & Root ------------------------------- //

bool lt_isLeaf_64 (in uvec2 nodeID)
{
    return (lt_level_64(nodeID) == 31u);
}

bool lt_isRoot_64 (in uvec2 nodeID)
{
    return (lt_findMSB_64(nodeID) == 0u);
}

// -------------------------------- Position -------------------------------- //

bool lt_isLeft_64 (in uvec2 nodeID)
{
    return !bool(nodeID.y & 0x1);
}

bool lt_isRight_64 (in uvec2 nodeID)
{
    return bool(nodeID.y & 0x1);
}

bool lt_isUpper_64 (in uvec2 nodeID)
{
    return !bool(nodeID.y & 0x2);
}

bool lt_isLower_64 (in uvec2 nodeID)
{
    return bool(nodeID.y & 0x2);
}

bool lt_isUpperLeft_64 (in uvec2 nodeID)
{
    return !(bool(nodeID.y & 0x3));
}


// ----------------------------- Triangle XForm ----------------------------- //

mat3 jk_bitToMatrix(in uint b)
{
    float bf = b;
    float s = 2.0 * bf - 1.0;
    return mat3(+s*0.5,   -0.5, 0.0,
                  -0.5, -s*0.5, 0.0,
                   0.5,    0.5, 1.0);
}

void lt_getTriangleXform_64 (in uvec2 nodeID, out mat3 xform, out mat3 parent_xform)
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


void lt_getTriangleXform_64 (in uvec2 nodeID, out mat3 xform)
{
    mat3 tmp;
    lt_getTriangleXform_64(nodeID, xform, tmp);
}

// -------------------------------- Mappings -------------------------------- //

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

// *** Get the given primitive *** //
void lt_getMeshTriangle(in uint meshPolygonID, out Triangle triangle)
{
    for (int i = 0; i < 3; ++i)
    {
        triangle.vertex[i].p = mesh_v[mesh_t_idx[meshPolygonID + i]].p;
        triangle.vertex[i].n = mesh_v[mesh_t_idx[meshPolygonID + i]].n;
        triangle.vertex[i].uv = mesh_v[mesh_t_idx[meshPolygonID + i]].uv;
    }
}

void lt_getMeshQuad(in uint meshPolygonID, out Quad quad)
{
    for (int i = 0; i < 4; ++i)
    {
        quad.vertex[i].p = mesh_v[mesh_q_idx[meshPolygonID + i]].p;
        quad.vertex[i].n = mesh_v[mesh_q_idx[meshPolygonID + i]].n;
        quad.vertex[i].uv = mesh_v[mesh_q_idx[meshPolygonID + i]].uv;
    }
}

void lt_getRootTriangle(in uint rootID, out Triangle2D t)
{
    uint b = rootID & 0x1;
    t.p[0] = vec2(b, b);
    t.p[1] = vec2(b, 1-b);
    t.p[2] = vec2(1-b, b);
}

// *** Get the given primitive mean normal *** //
vec4 lt_getMeanPrimNormal(in uvec4 key, in int poly_type) {
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

// *** Leaf to Tree vertex *** //
vec2 lt_Leaf_to_Tree_64(in vec2 p, in uvec2 nodeID, in bool parent)
{
    mat3 xf, pxf, xform;
    lt_getTriangleXform_64(nodeID, xf, pxf);
    xform = (parent) ? pxf : xf;
    return (xform * vec3(p, 1)).xy;
}

vec2 lt_Leaf_to_Tree_64(in vec2 p, uvec2 nodeID)
{
    return lt_Leaf_to_Tree_64(p, nodeID, false);
}

// *** Tree to Mesh primitive  (for triangles) *** //
vec4 lt_Tree_to_MeshTriangle(in vec2 p, in uint meshPolygonID)
{
    Triangle t;
    lt_getMeshTriangle(meshPolygonID, t);
    return lt_mapTo3DTriangle(t, p);
}

// *** Tree to Root vertex (for quads) *** //
vec2 lt_Tree_to_QuadRoot(in vec2 p, in uint rootID)
{
    Triangle2D t;
    lt_getRootTriangle(rootID, t);
    return lt_mapTo2DTriangle(t, p);
}

// *** Root to Mesh primitive (for quads) *** //

vec4 lt_Root_to_MeshQuad(in vec2 p, in uint meshPolygonID)
{
    Quad q;
    lt_getMeshQuad(meshPolygonID, q);
    return lt_mapTo3DQuad(q, p);
}

// ******* Complete transformation ******* //
vec4 lt_Leaf_to_MeshTriangle(in vec2 p, in uvec4 key, in bool parent)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    vec2 tmp = p;
    tmp = lt_Leaf_to_Tree_64(tmp, nodeID, parent);
    return lt_Tree_to_MeshTriangle(tmp, meshPolygonID);
}

vec4 lt_Leaf_to_MeshQuad(in vec2 p, in uvec4 key, in bool parent)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    vec2 tmp = p;
    tmp = lt_Leaf_to_Tree_64(tmp, nodeID, parent);
    tmp = lt_Tree_to_QuadRoot(tmp, rootID);
    return lt_Root_to_MeshQuad(tmp, meshPolygonID);
}

vec4 lt_Leaf_to_MeshPrimitive(in vec2 p, in uvec4 key, in bool parent, int poly_type)
{
    if (poly_type == TRIANGLES)
        return lt_Leaf_to_MeshTriangle(p, key, parent);
    else if (poly_type == QUADS)
        return lt_Leaf_to_MeshQuad(p, key, parent);
}

// ******* Complete transformation for both child and parent ******* //

void lt_Leaf_n_Parent_to_MeshTriangle(in vec2 p, in uvec4 key, out vec4 p_mesh, out vec4 pp_mesh)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    mat3 xf, pxf;
    vec2 p2D, pp2D;

    lt_getTriangleXform_64(nodeID, xf, pxf);
    p2D = (xf * vec3(p, 1)).xy;
    pp2D = (pxf * vec3(p, 1)).xy;

    p_mesh = lt_Tree_to_MeshTriangle(p2D, meshPolygonID);
    pp_mesh = lt_Tree_to_MeshTriangle(pp2D, meshPolygonID);
}

void lt_Leaf_n_Parent_to_MeshQuad(in vec2 p, in uvec4 key, out vec4 p_mesh, out vec4 pp_mesh)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    mat3 xf, pxf;
    vec2 p2D, pp2D;

    lt_getTriangleXform_64(nodeID, xf, pxf);
    p2D = (xf * vec3(p, 1)).xy;
    pp2D = (pxf * vec3(p, 1)).xy;

    p2D = lt_Tree_to_QuadRoot(p2D, rootID);
    pp2D = lt_Tree_to_QuadRoot(pp2D, rootID);

    p_mesh = lt_Root_to_MeshQuad(p2D, meshPolygonID);
    pp_mesh = lt_Root_to_MeshQuad(pp2D, meshPolygonID);
}

void lt_Leaf_n_Parent_to_MeshPrimitive(in vec2 p, in uvec4 key,
                                       out vec4 p_mesh,
                                       out vec4 pp_mesh,
                                       uint poly_type)
{
    if (poly_type == TRIANGLES)
        lt_Leaf_n_Parent_to_MeshTriangle(p, key, p_mesh, pp_mesh);
    else if (poly_type == QUADS)
        lt_Leaf_n_Parent_to_MeshQuad(p, key, p_mesh, pp_mesh);
}

// ******* Transformation to and from Tree ******* //
// Used to avoid unnecessary computations around the render pass morphing
vec4 lt_Tree_to_MeshTriangle(in vec2 p, in uvec4 key, in bool parent)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    vec2 tmp = p;
//    tmp = lt_Tree_to_TriangleRoot(p, rootID);
    return lt_Tree_to_MeshTriangle(tmp, meshPolygonID);
}

vec4 lt_Tree_to_MeshQuad(in vec2 p, in uvec4 key, in bool parent)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    vec2 tmp = p;
    tmp = lt_Tree_to_QuadRoot(p, rootID);
    return lt_Root_to_MeshQuad(tmp, meshPolygonID);
}

vec4 lt_Tree_to_MeshPrimitive(in vec2 p, in uvec4 key, in bool parent, int poly_type)
{
    if (poly_type == TRIANGLES)
        return lt_Tree_to_MeshTriangle(p, key, parent);
    else if (poly_type == QUADS)
        return lt_Tree_to_MeshQuad(p, key, parent);
}

#endif
