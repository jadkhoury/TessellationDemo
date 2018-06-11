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

// Functions
uvec4 lt_getKey_64(uint idx);
// Access parent / children nodes
void lt_children_32 (in uint nodeID, out uint children[4]);
void lt_children_64 (in uvec2 nodeID, out uvec2 children[4]);
uint lt_parent_32 (in uint nodeID);
uvec2 lt_parent_64 (in uvec2 nodeID);
// Evaluate level in key
uint lt_level_32 (in uint nodeID);
uint lt_level_64 (in uvec2 nodeID);
bool lt_isLeaf_32 (in uint nodeID);
bool lt_isLeaf_64 (in uvec2 nodeID);
bool lt_isRoot_32 (in uint nodeID);
bool lt_isRoot_64 (in uvec2 nodeID);
// Position tests for Quads
bool lt_isLeft_32 (in uint nodeID);
bool lt_isLeft_64 (in uvec2 nodeID);
bool lt_isRight_32 (in uint nodeID);
bool lt_isRight_64 (in uvec2 nodeID);
bool lt_isUpper_32 (in uint nodeID);
bool lt_isUpper_64 (in uvec2 nodeID);
bool lt_isLower_32 (in uint nodeID);
bool lt_isLower_64 (in uvec2 nodeID);
bool lt_isUpperLeft_32 (in uint nodeID);
bool lt_isUpperLeft_64 (in uvec2 nodeID);
// Position tests for Triangles
bool lt_isXborder_64 (in uvec2 nodeID);
bool lt_isYborder_64 (in uvec2 nodeID);
// Oddity
bool lt_isEven_64 (in uvec2 nodeID);
bool lt_isOdd_64 (in uvec2 nodeID);
// Access neighbour node
uvec4 lt_getLeafNeighbour_64(in uvec4 key, int axis, int prim_type);
uvec4 lt_adjacentRootNeighbour_64(in uvec4 key, in bvec2 border, int prim_type);
uvec4 lt_getLeafNeighbour_64(in uvec4 key, int axis, int prim_type);
// Set / Get morph bits
uvec4 lt_setXMorphBit_64(in uvec4 key);
bool lt_hasXMorphBit_64(in uvec4 key);
uvec4 lt_setYMorphBit_64(in uvec4 key);
bool lt_hasYMorphBit_64(in uvec4 key);
uvec4 lt_setDestroyBit_64(in uvec4 key);
bool lt_hasDestroyBit_64(in uvec4 key);
// Mappings
vec2 lt_Leaf_to_Tree_64(in vec2 p, in uvec2 nodeID, in bool parent, in bvec2 morph);
vec2 lt_Leaf_to_Tree_64(in vec2 p, in uvec2 nodID, in bool parent);
vec2 lt_Leaf_to_Tree_64(in vec2 p, uvec2 nodeID);
vec2 lt_Tree_to_QuadRoot(in vec2 p, uint rootID);
vec2 lt_Tree_to_TriangleRoot(in vec2 p, uint rootID);
vec4 lt_Root_to_MeshTriangle(in vec2 p, uint meshPolygonID);
vec4 lt_Root_to_MeshQuad(in vec2 p, uint meshPolygonID);
vec4 lt_Leaf_to_MeshTriangle(in vec2 p, uvec4 key, bool parent);
vec4 lt_Leaf_to_MeshTriangle(in vec2 p, uvec4 key);
vec4 lt_Leaf_to_MeshQuad(in vec2 p, uvec4 key, bool parent);
vec4 lt_Leaf_to_MeshQuad(in vec2 p, uvec4 key);


// ************************ IMPLEMENTATIONS ************************ //

uvec4 lt_getKey_64(uint idx)
{
    return nodes_in[idx];
}

// ------------------ Utility functions ------------------ //
mat3 lt_buildXformMatrix(in vec2 tr, in float scale, in float cosT, in float sinT)
{
    mat3 xform = mat3(1.0);
    if (cosT != 1 || sinT != 0) {
        //translation * scale * rotation =>
        // | sx*cosT  -sx*sinT  tr_x |
        // | sy*sinT   sy*cosT  tr_y |
        // |   0         0       1   |
        xform[0][0] = scale * cosT; xform[1][0] =-scale * sinT; xform[2][0] = tr.x;
        xform[0][1] = scale * sinT; xform[1][1] = scale * cosT; xform[2][1] = tr.y;
    } else {
        xform[0][0] = scale;
        xform[1][1] = scale;
        xform[2][0] = tr.x;
        xform[2][1] = tr.y;
        xform[2][2] = 1.0;
    }
    return xform;
}

mat3 lt_buildXformMatrix(in vec2 tr, in float scale)
{
    return lt_buildXformMatrix(tr, scale, 0, 1);
}

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

    result.y = result.y >> shift;
    result.y |= result.x << (32 - shift);
    result.x = result.x >> shift;

    return result;
}

int lt_findMSB_64(uvec2 nodeID)
{
    return (nodeID.x == 0) ? findMSB(nodeID.y) : (findMSB(nodeID.x) + 32);
}

// ------------- Children and Parents --------------- //

void lt_children_32 (in uint nodeID, out uint children[4])
{
    nodeID = nodeID << 2u;
    children[0] = nodeID;
    children[1] = nodeID | 0x1;
    children[2] = nodeID | 0x2;
    children[3] = nodeID | 0x3;
}

void lt_children_64 (in uvec2 nodeID, out uvec2 children[4])
{
    nodeID = lt_leftShift_64(nodeID, 2u);
    children[0] = nodeID;
    children[1] = uvec2(nodeID.x, nodeID.y | 0x1);
    children[2] = uvec2(nodeID.x, nodeID.y | 0x2);
    children[3] = uvec2(nodeID.x, nodeID.y | 0x3);
}


uint lt_parent_32 (in uint nodeID)
{
    return nodeID >> 2u;
}

uvec2 lt_parent_64 (in uvec2 nodeID)
{
    return lt_rightShift_64(nodeID, 2u);
}

// --------------------- Level ---------------------- //

uint lt_level_32 (in uint nodeID)
{
    uint i = findMSB(nodeID);
    return (i >> 1u);
}

uint lt_level_64 (in uvec2 nodeID)
{
    uint i = lt_findMSB_64(nodeID);
    return (i >> 1u);
}

// ------------------ Leaf & Root ------------------- //

bool lt_isLeaf_32 (in uint nodeID)
{
    return (findMSB(nodeID) == 15u);
}

bool lt_isLeaf_64 (in uvec2 nodeID)
{
    return (lt_level_64(nodeID) == 31u);
}

bool lt_isRoot_32 (in uint nodeID)
{
    return (findMSB(nodeID) == 0u);
}

bool lt_isRoot_64 (in uvec2 nodeID)
{
    return (lt_findMSB_64(nodeID) == 0u);
}

// -------------------- Position --------------------- //

// For Quads
bool lt_isLeft_32 (in uint nodeID)
{
    return !bool(nodeID & 0x1);
}
bool lt_isLeft_64 (in uvec2 nodeID)
{
    return !bool(nodeID.y & 0x1);
}

bool lt_isRight_32 (in uint nodeID)
{
    return bool(nodeID & 0x1);
}
bool lt_isRight_64 (in uvec2 nodeID)
{
    return bool(nodeID.y & 0x1);
}

bool lt_isUpper_32 (in uint nodeID)
{
    return !bool(nodeID & 0x2);
}
bool lt_isUpper_64 (in uvec2 nodeID)
{
    return !bool(nodeID.y & 0x2);
}

bool lt_isLower_32 (in uint nodeID)
{
    return bool(nodeID & 0x2);
}
bool lt_isLower_64 (in uvec2 nodeID)
{
    return bool(nodeID.y & 0x2);
}

bool lt_isUpperLeft_32 (in uint nodeID)
{
    return !(bool(nodeID & 0x3));
}
bool lt_isUpperLeft_64 (in uvec2 nodeID)
{
    return !(bool(nodeID.y & 0x3));
}

// For Triangles
bool lt_isXborder_64 (in uvec2 nodeID)
{
    return bool(nodeID.y & 0x2);
}

bool lt_isYborder_64 (in uvec2 nodeID)
{
    return !bool(nodeID.y & 0x2);
}

bool lt_isEven_64 (in uvec2 nodeID)
{
    return !bool(nodeID.y & 0x1);
}

bool lt_isOdd_64 (in uvec2 nodeID)
{
    return bool(nodeID.y & 0x1);
}

// **** Neighborhood implementation *** //

uint lt_getQi_64(in uvec2 nodeID, uint idx)
{
    if (idx < 16) {
        return ((0x3 << (2*idx)) & nodeID.y) >> (2*idx);
    } else if (idx < 31)  {
        idx -= 16;
        return ((0x3 << (2*idx)) & nodeID.x) >> (2*idx);
    }
}

uvec2 lt_setQi_64(in uvec2 nodeID, uint new_qi, uint idx)
{
    uvec2 result = nodeID;
    uint shift;
    if (idx < 16) {
        shift = 2*idx;
        uint mask = uint(-1) ^ (0x3 << shift);
        result.y = (result.y & mask) | (new_qi << shift);
    } else {
        shift = 2 * (idx % 16);
        uint mask = uint(-1) ^ (0x3 << shift);
        result.x = (result.x & mask) | (new_qi << shift);
    }
    return result;
}

bvec2 lt_isRootBorder_64(in uvec2 nodeID)
{
    uint lvl = lt_level_64 (nodeID);
    int idx = int(lvl - 1);
    uint previousQi = lt_getQi_64 (nodeID, idx);
    uint qi;
    bvec2 result = bvec2( previousQi >= 2,
                          previousQi < 2);
    idx--;
    while (idx > -1)
    {
        qi = lt_getQi_64(nodeID, idx);
        if( (previousQi & 0x1) != ((qi & 0x2) >> 1))
            return bvec2(false);
        idx--;
        previousQi = qi;
    }
    return result;
}


uvec2 lt_oppositeNodeID_64(uvec2 nodeID)
{
    uint lvl = lt_level_64 (nodeID);
    uint mask = (0x1 << (2 * (lvl%16))) - 1;
    uvec2 result;
    result.x = (lvl > 16) ? mask ^ nodeID.x : nodeID.x;
    result.y = (lvl > 16) ? uint(-1) ^ nodeID.y : mask ^ nodeID.y;
    return result;
}


uint lt_adjacentRootID(in uint rootID, in bvec2 border, in int prim_type)
{
    uint b1 = (rootID & 0x2) >> 1;
    uint b2 = (rootID & 0x1);
    if (prim_type == TRIANGLES) {
        if (border.x)
            rootID = b1 | (~(b1^b2) << 1);
        else if (border.y)
            rootID = (1 << rootID);
    } else if (prim_type == QUADS) {
        if (border.x)
            rootID = (0x1^b1) | (b2 << 1);
        else if (border.y)
            rootID = b1 | ((~b2) << 1);
    }
    rootID = rootID & 0x3;
    return rootID;
}

uvec4 lt_adjacentRootNeighbour_64(in uvec4 key, in bvec2 border, in int prim_type)
{
    uvec2 op = lt_oppositeNodeID_64 (key.xy);
    uint adjRoot = lt_adjacentRootID (key.w, border, prim_type);
    return uvec4(op.x, op.y, key.z, adjRoot);
}

uvec4 lt_getLeafNeighbour_64(in uvec4 key, int axis, int prim_type)
{
    int state;
    if (axis == X_AXIS)
        state = X_STATE;
    else if (axis == Y_AXIS)
        state = Y_STATE;

    uint digit = 0;
    uvec2 result = key.xy;
    uint lvl = lt_level_64(result);
    uint idx = 0;
    bvec2 out_of_root = bvec2(false);

    while (idx < lvl)
    {
        digit = lt_getQi_64(result, idx);
        if (idx == (lvl - 1)) {
            out_of_root.x = ((state == X_STATE && digit == 3) || (state == Y_STATE && digit == 2));
            out_of_root.y = ((state == X_STATE && digit == 1) || (state == Y_STATE && digit == 0));
            if (any(out_of_root)) {
                return  lt_adjacentRootNeighbour_64(key, out_of_root, prim_type);
            }
        }
        switch (state)
        {
        case X_STATE:
            if (digit == 0 || digit == 2) {
                state = INC_STATE;
            } else if (digit == 1) {
                result = lt_setQi_64(result, 2, idx);
                state = Y_STATE;
                idx++;
            } else if (digit == 3) {
                result = lt_setQi_64(result, 0, idx);
                idx++;
            }
            break;

        case Y_STATE:
            if (digit == 1 || digit == 3) {
                state = DEC_STATE;
            } else if (digit == 0) {
                result = lt_setQi_64(result, 3, idx);
                idx++;
            } else if (digit == 2) {
                result = lt_setQi_64(result, 1, idx);
                state = X_STATE;
                idx++;
            }
            break;

        case INC_STATE:
            if (digit == 3) {
                result = lt_setQi_64(result, 0, idx);
                idx++;
            } else {
                result = lt_setQi_64(result, digit+1, idx);
                idx = lvl + 1;
            }
            break;

        case DEC_STATE:
            if (digit == 0) {
                result = lt_setQi_64(result, 3, idx);
                idx++;
            } else {
                result = lt_setQi_64(result, digit-1, idx);
                idx = lvl + 1;
            }
            break;

        default:
            break;
        }
    }
    return uvec4(result.x, result.y, key.z, key.w);
}

// ---------------------- Set / Check key morph status --------------------- //
uvec4 lt_setXMorphBit_64(in uvec4 key)
{
    uvec4 r = key;
    r.w |= (1 << 31);
    return r;
}

bool lt_hasXMorphBit_64(in uvec4 key)
{
    return bool(key.w & (1 << 31));
}

uvec4 lt_setYMorphBit_64(in uvec4 key)
{
    uvec4 r = key;
    r.w |= (1 << 30);
    return r;
}

bool lt_hasYMorphBit_64(in uvec4 key)
{
    return bool(key.w & (1 << 30));
}

uvec4 lt_setDestroyBit_64(in uvec4 key)
{
    uvec4 r = key;
    r.w |= (1 << 29);
    return r;
}

bool lt_hasDestroyBit_64(in uvec4 key)
{
    return bool(key.w & (1 << 29));
}


// key:  ---- ---- ---- --01 b1b2-- ---- ---- ----
// mask: ---- ---- ---- ----  11--  ---- ---- ----
// ----------------- Triangle XForm ------------------ //

void lt_getTriangleXform_64 (in uvec2 key, out mat3 xform, out mat3 parent_xform, in bvec2 morph)
{
#if 0
    uint current_half = (key.x == 0) ? key.y : key.x;
    bool round2 = (key.x == 0) ? true : false;
    vec2 tr = vec2(0.0);
    float scale = 1.0f;
    int shift = int(findMSB(current_half) - 2);
    uint mask, b1b2;
    uint b1, b2;
    float theta = 0;
    float cosT = 1, sinT = 0;
    float old_cosT = 1;
    float tmp_x, tmp_y;
    float a = 1, b = 0;

    if (key.x == 0x1) {
        round2 = true;
        shift = 30;
        current_half = key.y;
    }
    while (shift >= 0)
    {
        if (round2 && shift == 0)
            parent_xform = lt_buildXformMatrix(tr, scale, cosT, sinT);
        mask = 0x3 << shift;
        b1b2 = (current_half & mask) >> shift;
        b1 = (b1b2 & 0x2) >> 1u;
        b2 = b1b2 & 0x1;

        //Last morphed transfo
        if (shift == 0 && any(morph))
        {
            scale *= SQRT_2_INV;
            tmp_x = scale * SQRT_2_INV;
            tr.x += cosT*tmp_x - sinT*tmp_x;
            tr.y += sinT*tmp_x + cosT*tmp_x;
            a = - SQRT_2_INV;
            if (morph.x) {
                cosT = a*cosT     - a*sinT;
                sinT = a*old_cosT + a*sinT;
            } else if (morph.y) {
                b =   SQRT_2_INV;
                cosT = a*cosT     - b*sinT;
                sinT = b*old_cosT + a*sinT;
            }
            xform = lt_buildXformMatrix(tr, scale, cosT, sinT);
            return;
        }
        scale *= 0.5;
        tmp_x = scale * float(b1 & 0x1);
        tmp_y = scale * float(b1 ^ 0x1);

        tr.x += cosT*tmp_x - sinT*tmp_y;
        tr.y += sinT*tmp_x + cosT*tmp_y;

        a = int((b1 ^ b2) ^ 0x1);
        b = a + int(((b1 ^ b2) & b1) << 1) - 1;
        cosT = a*cosT - b*sinT;
        sinT = b*old_cosT + a*sinT;

        old_cosT = cosT;

        shift -= 2;

        if (shift < 0 && !round2) {
            current_half = key.y;
            shift = 30;
            round2 = true;
        }
    }
    xform = lt_buildXformMatrix(tr, scale, cosT, sinT);

#else
    uint b, s;
    mat3 xf = mat3(1.0);
    mat3 last_xf;

    mat3  x_0 = mat3(-0.5,  -0.5, 0.5,
                     -0.5,   0.5, 0.5,
                      0,     0,   1);


    mat3 x_1 = mat3( 0.5, -0.5, 0.5,
                     -0.5, -0.5, 0.5,
                     0,    0,   1);
    x_0 = transpose(x_0);
    x_1 = transpose(x_1);

    mat3 current_x;
    bool first = true;

    b = key.y & 0x1;
    last_xf = (b == 0) ? x_0 : x_1;
//    key = lt_rightShift_64(key, 1);

    while (key.x > 0 || key.y > 1)
    {
        b = key.y & 0x1;
        current_x = (b == 0) ? x_0 : x_1;
        xf = current_x * xf;
        key = lt_rightShift_64(key, 1);
    }
    parent_xform = xf * inverse(last_xf);
    xform = xf;
#endif

}

void lt_getTriangleXform_64 (in uvec2 nodeID, out mat3 xform, out mat3 parent_xform)
{
    lt_getTriangleXform_64(nodeID, xform, parent_xform, bvec2(false));
}

void lt_getTriangleXform_64 (in uvec2 nodeID, out mat3 xform, in bvec2 morph)
{
    mat3 tmp;
    lt_getTriangleXform_64(nodeID, xform, tmp, morph);
}

void lt_getTriangleXform_64 (in uvec2 nodeID, out mat3 xform)
{
    mat3 tmp;
    lt_getTriangleXform_64(nodeID, xform, tmp, bvec2(false));
}


void lt_getTriangleXform_32(in uint nodeID, out mat3 xform, out mat3 parent_xform)
{
    uvec2 new_nodeID = uvec2(0u, nodeID);
    lt_getTriangleXform_64(new_nodeID, xform, parent_xform, bvec2(false));
}

void lt_getTriangleXform_32(in uint nodeID, out mat3 xform)
{
    mat3 tmp;
    uvec2 new_nodeID = uvec2(0u, nodeID);
    lt_getTriangleXform_64(new_nodeID, xform, tmp, bvec2(false));
}


// ----------------------- Mappings ----------------------- //

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
    //return vec4(mapped, 1.0);
}

vec4 lt_mapTo3DQuad(Quad q, vec2 uv)
{
    vec4 p01 = mix(q.vertex[0].p, q.vertex[1].p, uv.x);
    vec4 p32 = mix(q.vertex[3].p, q.vertex[2].p, uv.x);
    return mix(p01, p32, uv.y);
    //	vec4 mixed = mix(p01, p32, uv.y);
    //	return vec4(mixed, 1.0);
}

// *** Get the given primitive *** //
void lt_getMeshTriangle(in uint meshPolygonID, out Triangle triangle)
{
    Vertex v;
    for (int i = 0; i < 3; ++i)
    {
        v.p = mesh_v[mesh_t_idx[meshPolygonID + i]].p;
        v.n = mesh_v[mesh_t_idx[meshPolygonID + i]].n;
        v.uv = mesh_v[mesh_t_idx[meshPolygonID + i]].uv;
        triangle.vertex[i] = v;
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

void lt_getRootTriangle(in uint rootID, in int prim_type, out Triangle2D t)
{
    if (prim_type == TRIANGLES) {
        uint b1b2 = rootID & 0x3;
        uint b1 = (b1b2 & 0x2) >> 1;
        uint b2 = (b1b2 & 0x1);
        t.p[0] = vec2(1.0/3.0, 1.0/3.0);
        t.p[1] = vec2(b2, 1-(b1^b2));
        t.p[2] = vec2(b1, b2);
    } else if (prim_type == QUADS) {
        uint b1b2 = rootID & 0x3;
        uint b1 = (b1b2 & 0x2) >> 1;
        uint b2 = (b1b2 & 0x1);
        t.p[0] = vec2(0.5, 0.5);
        t.p[1] = vec2(b1, b2);
        t.p[2] = vec2(b2, 1-b1);
    }
}

// *** Get the given primitive mean normal *** //
vec4 lt_getMeanPrimNormal(in uvec4 key, in int prim_type) {
    uint meshPolygonID = key.z;
    vec4 sum = vec4(0);

    if (prim_type == TRIANGLES)
        for (int i = 0; i < 3; ++i)
            sum += mesh_v[mesh_t_idx[meshPolygonID + i]].n;

    else if (prim_type == QUADS)
        for (int i = 0; i < 4; ++i)
            sum += mesh_v[mesh_q_idx[meshPolygonID + i]].n;

    sum = normalize(sum);
    return (sum == vec4(0)) ? vec4(0,1,0,0) : sum;
}

// *** Leaf to Tree vertex *** //
vec2 lt_Leaf_to_Tree_64(in vec2 p, in uvec2 nodeID, in bool parent, in bvec2 morph)
{
    mat3 xform;
    if(parent){
        mat3 tmp;
        lt_getTriangleXform_64(nodeID, tmp, xform, morph);
    } else {
        lt_getTriangleXform_64(nodeID, xform, morph);
    }
    return (xform * vec3(p, 1)).xy;
}

vec2 lt_Leaf_to_Tree_64(in vec2 p, in uvec2 nodeID, in bool parent)
{
    mat3 xform;
    if(parent){
        mat3 tmp;
        lt_getTriangleXform_64(nodeID, tmp, xform);
    } else {
        lt_getTriangleXform_64(nodeID, xform);
    }
    return  (xform * vec3(p, 1)).xy;
}

vec2 lt_Leaf_to_Tree_64(in vec2 p, uvec2 nodeID)
{
    return lt_Leaf_to_Tree_64(p, nodeID, false);
}

// *** Tree to Root vertex *** //
vec2 lt_Tree_to_QuadRoot(in vec2 p, in uint rootID)
{
    Triangle2D t;
    lt_getRootTriangle(rootID, QUADS, t);
    return lt_mapTo2DTriangle(t, p);
}

vec2 lt_Tree_to_TriangleRoot(in vec2 p, in uint rootID)
{
    Triangle2D t;
    lt_getRootTriangle(rootID, TRIANGLES, t);
    return lt_mapTo2DTriangle(t, p);
}

// *** Root to Mesh primitive *** //
vec4 lt_Root_to_MeshTriangle(in vec2 p, in uint meshPolygonID)
{
    Triangle t;
    lt_getMeshTriangle(meshPolygonID, t);
    return lt_mapTo3DTriangle(t, p);
}

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
    bvec2 morph = bvec2(lt_hasXMorphBit_64(key), lt_hasYMorphBit_64(key));
    vec2 tmp = p;
    tmp = lt_Leaf_to_Tree_64(tmp, nodeID, parent, morph);
    tmp = lt_Tree_to_TriangleRoot(tmp, rootID);
    return lt_Root_to_MeshTriangle(tmp, meshPolygonID);
}

vec4 lt_Leaf_to_MeshQuad(in vec2 p, in uvec4 key, in bool parent)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    bvec2 morph = bvec2(lt_hasXMorphBit_64(key), lt_hasYMorphBit_64(key));
    vec2 tmp = p;
    tmp = lt_Leaf_to_Tree_64(tmp, nodeID, parent, morph);
    tmp = lt_Tree_to_QuadRoot(tmp, rootID);
    return lt_Root_to_MeshQuad(tmp, meshPolygonID);
}

vec4 lt_Leaf_to_MeshPrimitive(in vec2 p, in uvec4 key, in bool parent, int prim_type)
{
    if (prim_type == TRIANGLES)
        return lt_Leaf_to_MeshTriangle(p, key, parent);
    else if (prim_type == QUADS)
        return lt_Leaf_to_MeshQuad(p, key, parent);
}

vec4 lt_Tree_to_MeshTriangle(in vec2 p, in uvec4 key, in bool parent)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    bvec2 morph = bvec2(false);
    vec2 tmp = lt_Tree_to_TriangleRoot(p, rootID);
    return lt_Root_to_MeshTriangle(tmp, meshPolygonID);
}

vec4 lt_Tree_to_MeshQuad(in vec2 p, in uvec4 key, in bool parent)
{
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    bvec2 morph = bvec2(lt_hasXMorphBit_64(key), lt_hasYMorphBit_64(key));
    vec2 tmp = lt_Tree_to_QuadRoot(p, rootID);
    return lt_Root_to_MeshQuad(tmp, meshPolygonID);
}



vec4 lt_Tree_to_MeshPrimitive(in vec2 p, in uvec4 key, in bool parent, int prim_type)
{
    if (prim_type == TRIANGLES)
        return lt_Tree_to_MeshTriangle(p, key, parent);
    else if (prim_type == QUADS)
        return lt_Tree_to_MeshQuad(p, key, parent);
}

#endif
