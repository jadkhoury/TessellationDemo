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

uniform int cull;

uniform int copy_pass;

uniform float cpu_lod;

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

vec4 heightDisplace(in vec4 v, in vec4 n) {
    vec4 r = v;
    r.z = displace(r.xyz, 1000, n.xyz) * 2.0;
    return r;
}

////////////////////////////////////////////////////////////////////////////////
///
/// COMPUTE PASS FUNCTIONS
///

/**
 * Copy the new nodeID and old primitive index in the new key
 * Extract the root ID from the w component (i.e. don't copy the morph / destroy bit)
 * The z coordinate corresponding to the mesh primitive is left unmodified since
 * all parents and children of a leaf always lie on the same mesh primitive
 * Store the resulting key in the SBBO for the next compute pass
 * Increments the dispatch counter every n times such that we only use enough Workgroups
 */
void compute_writeKey(uvec2 new_nodeID, uvec4 current_key)
{
    uvec4 new_key = uvec4(new_nodeID, current_key.z, (current_key.w & 0x3));
    uint idx = atomicCounterIncrement(primCount_full[write_index]);
    nodes_out_full[idx] = new_key;
}

void compute_writeChildren(uvec2 children[4], uvec4 current_key)
{
    for(int i = 0; i<4; ++i) {
        uint idx = atomicCounterIncrement(primCount_full[write_index]);
        uvec4 new_key = uvec4(children[i], current_key.z, (current_key.w & 0x3));
        nodes_out_full[idx] = new_key;
    }
}

/* Emulates what was previously the Compute Pass:
 * - Compute the LoD stored in the key
 * - Decides wether to merge, divide or just pass forward the current leaf
 *		- if uniform  subdivision: criterion with target level set by user
 *		- if adaptive subdivision: criterion using the LoD functions
 * - Store the obtained key(s) in the SSBO for the next compute pass
 */
void computePass(uvec4 key, uint invocation_idx)
{
    uvec2 nodeID = key.xy;
    uint current_lvl = lt_level_64(nodeID);


    // Check if a merge or division is required
    bool should_divide, should_merge;
    if (uniform_subdiv > 0) {
        should_divide = current_lvl < uniform_level;
        should_merge = current_lvl > uniform_level;
    } else {
        float parentTargetLevel, targetLevel;
        computeTessLvlWithParent(key, targetLevel, parentTargetLevel);

        should_divide = float(current_lvl) < targetLevel;
        should_merge  = float(current_lvl) >= parentTargetLevel + 1.0;
    }

    // Perform merge, subdiv, or passthrough
    if (should_divide && !lt_isLeaf_64(nodeID)) {
        uvec2 childrenID[4];
        lt_children_64(nodeID, childrenID);
        compute_writeChildren(childrenID, key);
    } else if (should_merge && !lt_isRoot_64(nodeID)) {
        if (lt_isUpperLeft_64(nodeID)) {
            uvec2 parentID = lt_parent_64(nodeID);
            compute_writeKey(parentID, key);
        }
    } else {
        compute_writeKey(nodeID, key);
    }
}

////////////////////////////////////////////////////////////////////////////////
///
/// CULL PASS FUNCTIONS
///

// Store the new key in the Culled SSBO for the Render Pass
void cull_writeKey(uvec4 new_key)
{
    uint idx = atomicCounterIncrement(primCount_culled[write_index]);
    nodes_out_culled[idx] =  new_key;
}

/**
 * Emulates what was previously the Cull Pass:
 * - Compute the mesh space bounding box of the primitive
 * - Use the BB to check for culling
 * - If not culled, store the (possibly morphed) key in the SSBO for Render
 */
void cullPass(uvec4 key)
{
    if(cull > 0)
    {
        mat4 mesh_coord;
        mesh_coord[O] = lt_Leaf_to_MeshPrimitive(unit_O, key, false, prim_type);
        mesh_coord[U] = lt_Leaf_to_MeshPrimitive(unit_U, key, false, prim_type);
        mesh_coord[R] = lt_Leaf_to_MeshPrimitive(unit_R, key, false, prim_type);

        if (heightmap > 0) {
            vec4 n = vec4(0,0,1,0);
            mesh_coord[O] = heightDisplace(mesh_coord[O], n);
            mesh_coord[U] = heightDisplace(mesh_coord[U], n);
            mesh_coord[R] = heightDisplace(mesh_coord[R], n);
        }

        vec4 b_min = vec4(10e6);
        b_min = min(b_min, mesh_coord[O]);
        b_min = min(b_min, mesh_coord[U]);
        b_min = min(b_min, mesh_coord[R]);

        vec4 b_max = vec4(-10e6);
        b_max = max(b_max, mesh_coord[O]);
        b_max = max(b_max, mesh_coord[U]);
        b_max = max(b_max, mesh_coord[R]);

        if (culltest(MVP, b_min.xyz, b_max.xyz))
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
        active_nodes = max(num_mesh_quad * 2, int(atomicCounter(primCount_full[read_index])));
    } else if (prim_type ==  TRIANGLES) {
        active_nodes = max(num_mesh_tri, int(atomicCounter(primCount_full[read_index])));
    }
    if (invocation_idx >= active_nodes)
        return;

    computePass(key, invocation_idx);
    cullPass(key);

    return;
}

#endif
