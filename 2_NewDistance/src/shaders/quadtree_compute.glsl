#line 2

#ifdef COMPUTE_SHADER

/**
 * In LoD:
 * uniform int prim_type;
 * uniform vec3 cam_pos;
 * uniform mat4 M, V, P, MV, MVP;
 */

layout (local_size_x = LOCAL_WG_SIZE_X,
        local_size_y = LOCAL_WG_SIZE_Y,
        local_size_z = LOCAL_WG_SIZE_Z) in;

layout (binding = NODECOUNTER_FULL_B)   uniform atomic_uint primCount_full[16];
layout (binding = NODECOUNTER_CULLED_B) uniform atomic_uint primCount_culled[16];

uniform int read_index, write_index;

uniform int uniform_level;
uniform int uniform_subdiv;

uniform int num_mesh_tri;
uniform int num_mesh_quad;

uniform int morph;
uniform int morph_mode;

uniform int cull;

uniform int copy_pass;

uniform float cpu_lod;
uniform float edgePixelLengthTarget;
const float screenResolution = 1024.0;

uniform int heightmap;


/**
 *   U
 *   |\
 *   | \
 *   |  \
 *   |___\
 *   O    R
 * (Up, Origin, Right)
 *
 */

const int O = 0;
const int R = 1;
const int U = 2;
const vec2 unit_O = vec2(0,0);
const vec2 unit_R = vec2(1,0);
const vec2 unit_U = vec2(0,1);


bool outside_frustum = false;

#define OLD_LOD

// Computes a simple point symetry
vec4 Reflect (vec4 p, vec4 o) {
    return 2*o-p;
}

vec4 heightDisplace(in vec4 v, in vec4 n) {
    vec4 r = v;
    r.z = displace(r.xyz, 1000, n.xyz) * 2.0;
    return r;
}

float computeTessLevelFromUnit(vec2 p0, vec2 p1, uvec4 key, bool parent)
{
    vec4 mesh_p0 = lt_Leaf_to_MeshPrimitive(p0, key, parent, prim_type);
    vec4 mesh_p1 = lt_Leaf_to_MeshPrimitive(p1, key, parent, prim_type);

    if (heightmap > 0) {
        vec4 n = vec4(0,0,1,0);
        mesh_p0 = heightDisplace(mesh_p0, n);
        mesh_p1 = heightDisplace(mesh_p1, n);
    }

    vec4 mv_p0 = MV * mesh_p0;
    vec4 mv_p1 = MV * mesh_p1;
    return lt_computeTessLevel_64(mesh_p0, mesh_p1, mv_p0, mv_p1);
}

float computeTessLevelFromKey(uvec4 key, bool parent) {
    return computeTessLevelFromUnit(unit_U, unit_R, key, parent);

}

bool insideFrustum(mat4 mesh_coord)
{
    vec4 b_min = vec4(10e6);
    b_min = min(b_min, mesh_coord[O]);
    b_min = min(b_min, mesh_coord[U]);
    b_min = min(b_min, mesh_coord[R]);

    vec4 b_max = vec4(-10e6);
    b_max = max(b_max, mesh_coord[O]);
    b_max = max(b_max, mesh_coord[U]);
    b_max = max(b_max, mesh_coord[R]);
    return dcf_cull(MVP, b_min.xyz, b_max.xyz);
}

// ************************* COMPUTE PASS FUNCTIONS ************************** //

/**
 * Copy the new nodeID and old primitive index in the new key
 * Extract the root ID from the w component (i.e. don't copy the morph / destroy bit)
 * The z coordinate corresponding to the mesh primitive is left unmodified since
 * all parents and children of a leaf always lie on the same mesh primitive
 * Store the resulting key in the SBBO for the next compute pass
 * Increments the dispatch counter every n times such that we only use enough Workgroups
 */
void compute_writeKey(uvec2 new_nodeID, uint in_idx)
{
    uvec4 new_key = uvec4(new_nodeID, nodes_in[in_idx].z, (nodes_in[in_idx].w & 0x3));
    uint idx = atomicCounterIncrement(primCount_full[write_index]);
    nodes_out_full[idx] = new_key;
}


/* Emulates what was previously the Compute Pass:
 * - Compute the LoD stored in the key
 * - Decides wether to merge, divide or just pass forward the current leaf
 *		- if uniform  subdivision: criterion with target level set by user
 *		- if adaptive subdivision: criterion using the LoD functions
 * - Store the obtained key(s) in the SSBO for the next compute pass
 */
void computePass(uvec4 key, uint invocation_idx, mat4 mv_coord, mat4 mesh_coord)
{
    // Check if a merge or division is required
    uvec2 nodeID = key.xy;
    uint current_lvl = lt_level_64(nodeID);

    bool should_divide, should_merge;
    if (uniform_subdiv > 0) {
        should_divide = current_lvl < uniform_level;
        should_merge = current_lvl > uniform_level;
    } else {
        float parentLevel = computeTessLevelFromKey(key, true);
        float targetLevel =  lt_computeTessLevel_64(mesh_coord[U].xyz, mesh_coord[R].xyz,
                                                    mv_coord[U].xyz, mv_coord[R].xyz);
        should_divide = float(current_lvl) < targetLevel;
        should_merge  = float(current_lvl) >= parentLevel + 1.0;
    }

    if (should_divide && !lt_isLeaf_64(nodeID) && insideFrustum(mesh_coord)) {
        uvec2 childrenID[4];
        lt_children_64(nodeID, childrenID);
        for (int i = 0; i < 4; ++i)
            compute_writeKey(childrenID[i], invocation_idx);
    } else if (should_merge && !lt_isRoot_64(nodeID)) {
        //Merge
        if (lt_isUpperLeft_64(nodeID)) {
            uvec2 parentID = lt_parent_64(nodeID);
            compute_writeKey(parentID, invocation_idx);
        }
    } else {
        compute_writeKey(nodeID, invocation_idx);
    }
}


// *************************** CULL PASS FUNCTIONS *************************** //

/**
 * Returns the neighbour's LoD by reflecting the hypotenuse in unit space, mapping it
 * to world space and THEN computing the LoD
 * (PreMapping i.e. the reflection happens in unit space before the mappings)
*/
float getReflectedLOD_Hybrid_PreMapping(in uvec4 key, bool leaf_Xborder, bool leaf_Yborder)
{
    bvec2 root_border = lt_isRootBorder_64(key.xy);
    vec2 p0, p1;
    if (any(root_border)) {
        key = lt_adjacentRootNeighbour_64(key, root_border, prim_type);
        p0 = unit_U;
        p1 = unit_R;
    } else if (leaf_Xborder) {
        p0 = vec2(0, -1);
        p1 = unit_R;
    } else if (leaf_Yborder) {
        p0 = unit_U;
        p1 = vec2(-1, 0);
    }
    return computeTessLevelFromUnit(p0, p1, key, true);
}

/**
 * Returns the neighbour's LoD by reflecting the hypotenuse in world space and
 * computing the LoD
 * (PostMapping i.e. the reflection happens in world space after the mappings)
 */
float getReflectedLOD_Hybrid_PostMapping(uvec4 key, mat4 mv_coord)
{
    bvec2 root_border = lt_isRootBorder_64(key.xy);
    if (any(root_border))
    {
        uvec4 test_key = lt_adjacentRootNeighbour_64(key, root_border, prim_type);
        return computeTessLevelFromUnit(unit_U, unit_R, test_key, true);
    }
    else
    {
        uint lastBranch = key.y & 0x3;
        vec4 t, h0_mv, h1_mv;
        switch (lastBranch)
        {
        case 0:
            h0_mv = mv_coord[U];
            t = Reflect(mv_coord[R], mv_coord[O]);
            break;
        case 1:
            h0_mv = Reflect(mv_coord[R], mv_coord[O]);
            t = Reflect(mv_coord[U], mv_coord[O]);
            break;
        case 2:
            h0_mv = Reflect(mv_coord[U], mv_coord[O]);
            t = Reflect(mv_coord[R], mv_coord[O]);
            break;
        case 3:
            h0_mv = mv_coord[R];
            t = Reflect(mv_coord[U], mv_coord[O]);
            break;
        default:
            break;
        }
        h1_mv = Reflect(h0_mv, t);

        vec4 h0 = invMV * h0_mv;
        vec4 h1 = invMV * h1_mv;

        return lt_computeTessLevel_64(h0.xyz, h1.xyz, h0_mv.xyz, h1_mv.xyz);
    }
}
/**
 * Returns the neighbour's LoD by computing the nodeID and rootID of the relevant
 * neighbour, mapping the unit space hypotenuse using this key and computing the
 * LoD using this hypotenuse
 * (PostMapping i.e. the reflection happens in world space after the mappings)
 */
float getReflectedLOD_Neighbour(uvec4 key, bool leaf_Xborder, bool leaf_Yborder)
{
    uvec4 test_key;
    test_key = uvec4(lt_parent_64(key.xy), key.zw);
    int axis = (leaf_Xborder) ? X_AXIS : Y_AXIS;
    test_key = lt_getLeafNeighbour_64(test_key, axis, prim_type);
    //    return lt_computeTessLevelFromUnit_64(unit_U, unit_R, test_key, false);
    return computeTessLevelFromKey(test_key, false);
}


/**
 * Check if a given leaf should be morphed along the X or Y axis, should be destroyed
 * or should be left unmodified.
 */
uvec4 checkMorph (in uvec4 key, mat4 mv_coord)
{
    if (lt_isRoot_64(key.xy))
        return key;

    float reflect_lod;
    bool leaf_Xborder = lt_isXborder_64(key.xy);
    bool leaf_Yborder = lt_isYborder_64(key.xy);

    switch (morph_mode)
    {
    case NEIGHBOUR :
        reflect_lod = getReflectedLOD_Neighbour(key, leaf_Xborder, leaf_Yborder);
        break;
    case HYBRID_PRE:
        reflect_lod = getReflectedLOD_Hybrid_PreMapping(key, leaf_Xborder, leaf_Yborder);
        break;
    case HYBRID_POST:
        reflect_lod = getReflectedLOD_Hybrid_PostMapping(key, mv_coord);
        break;
    }


    if (reflect_lod > (float(lt_level_64(key.xy)) - 1.0)) {
        return key;
    } else if (lt_isOdd_64(key.xy)) {
        return lt_setDestroyBit_64(key);
    } else if (leaf_Xborder) {
        return lt_setXMorphBit_64(key);
    } else if (leaf_Yborder) {
        return lt_setYMorphBit_64(key);
    }
}



// Store the new key (after morph check) in the Culled SSBO for the Render Pass
void cull_writeKey(uvec4 new_key)
{
    uint idx = atomicCounterIncrement(primCount_culled[write_index]);
    nodes_out_culled[idx] =  new_key;
}
/**
 * Emulates what was previously the Cull Pass:
 * - Check if some morph or deletion should be done
 * - If current leaf is destroyed: returns before doing anything
 * - Else:
 *		- Compute the mesh space bounding box of the primitive
 *		- Use the BB to check for culling
 *		- If not culled, store the (possibly morphed) key in the SSBO for Render
 * TRICK: if both z coordinates are 0, assume that we're treating a grid+heightmap
 * situation, modify bounding box z coordinates to include a safe height margin.
 */
void cullPass(uvec4 key, mat4 mesh_coord, mat4 mv_coord)
{
    if (morph > 0) {
        key = checkMorph(key, mv_coord);
        if (lt_hasDestroyBit_64(key))
            return;
    }

    if(cull > 0) {
        vec4 b_min = vec4(10e6);
        b_min = min(b_min, mesh_coord[O]);
        b_min = min(b_min, mesh_coord[U]);
        b_min = min(b_min, mesh_coord[R]);

        vec4 b_max = vec4(-10e6);
        b_max = max(b_max, mesh_coord[O]);
        b_max = max(b_max, mesh_coord[U]);
        b_max = max(b_max, mesh_coord[R]);


        if (dcf_cull(MVP, b_min.xyz, b_max.xyz))
            cull_writeKey(key);
    } else {
        cull_writeKey(key);
    }
}

// *********************************** MAIN *********************************** //

void main(void)
{
    uint invocation_idx = int(gl_GlobalInvocationID.x);
    uvec4 key = lt_getKey_64(invocation_idx);
    // Removing morph / destroy bit
    key.w = key.w & 0x3;

    // Check if the current instance should work
    int active_nodes;

    if (prim_type == QUADS) {
        active_nodes = max(num_mesh_quad*4, int(atomicCounter(primCount_full[read_index])));
    } else if (prim_type ==  TRIANGLES) {
        active_nodes = max(num_mesh_tri*3, int(atomicCounter(primCount_full[read_index])));
    }
    if (invocation_idx >= active_nodes)
        return;

    mat4 mesh_coord, mv_coord;

    mesh_coord[O] = lt_Leaf_to_MeshPrimitive(unit_O, key, false, prim_type);
    mesh_coord[U] = lt_Leaf_to_MeshPrimitive(unit_U, key, false, prim_type);
    mesh_coord[R] = lt_Leaf_to_MeshPrimitive(unit_R, key, false, prim_type);

    if (heightmap > 0) {
        vec4 n = vec4(0,0,1,0);
        mesh_coord[O] = heightDisplace(mesh_coord[O], n);
        mesh_coord[U] = heightDisplace(mesh_coord[U], n);
        mesh_coord[R] = heightDisplace(mesh_coord[R], n);
    }

    mv_coord[O] = MV * mesh_coord[O];
    mv_coord[U] = MV * mesh_coord[U];
    mv_coord[R] = MV * mesh_coord[R];

    computePass(key, invocation_idx, mv_coord, mesh_coord);
    cullPass(key, mesh_coord, mv_coord);

    return;
}

#endif
