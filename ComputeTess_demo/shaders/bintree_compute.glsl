#line 2

#ifdef COMPUTE_SHADER

layout (local_size_x = LOCAL_WG_SIZE_X,
        local_size_y = LOCAL_WG_SIZE_Y,
        local_size_z = LOCAL_WG_SIZE_Z) in;

layout (binding = NODECOUNTER_FULL_B)   uniform atomic_uint nodeCount_full[2];
layout (binding = NODECOUNTER_CULLED_B) uniform atomic_uint nodeCount_culled[2];

shared float cam_height_local;

uniform int u_read_index;

uniform int u_uniform_subdiv;
uniform int u_uniform_level;

uniform int u_num_mesh_tri;
uniform int u_num_mesh_quad;

uniform int u_screen_res;

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

////////////////////////////////////////////////////////////////////////////////
///
/// COMPUTE PASS FUNCTIONS
///

/**
 * Copy the new nodeID and old primitive index in the new key
 * Extract the root ID from the w component
 * The z coordinate corresponding to the mesh primitive is left unmodified since
 * all parents and children of a leaf always lie on the same mesh primitive
 * Store the resulting key in the SBBO for the next compute pass
 * Increments the dispatch counter every n times such that we only use enough Workgroups
 */
void compute_writeKey(uvec2 new_nodeID, uvec4 current_key)
{
    uvec4 new_key = uvec4(new_nodeID, current_key.zw);
    uint idx = atomicCounterIncrement(nodeCount_full[1-u_read_index]);
    u_SubdBufferOut[idx] = new_key;
}

/**
 * Writes the keys in the buffer as dictated by the merge / split operators
 */

void updateSubdBuffer(uvec4 key, int targetLod, int parentLod)
{
    // extract subdivision level associated to the key
    uvec2 nodeID = key.xy;

    int keyLod = int(lt_level_64(nodeID));

    // update the key accordingly
    if (/* subdivide ? */ keyLod < targetLod && !lt_isLeaf_64(nodeID)) {
        uvec2 children[2]; lt_children_64(nodeID, children);
        compute_writeKey(children[0], key);
        compute_writeKey(children[1], key);
    } else if (/* keep ? */ keyLod < (parentLod + 1)) {
        compute_writeKey(nodeID, key);
    } else /* merge ? */ {
        if (/* is root ? */lt_isRoot_64(nodeID)) {
            compute_writeKey(nodeID, key);
        } else if (/* is zero child ? */lt_isZeroChild_64(nodeID)) {
            compute_writeKey(lt_parent_64(nodeID), key);
        }
    }
}

/* Emulates what was previously the Compute Pass:
 * - Compute the LoD stored in the key
 * - Decides wether to merge, divide or just pass forward the current leaf
 *		- if uniform  subdivision: criterion with target level set by user
 *		- if adaptive subdivision: criterion using the LoD functions
 * - Store the obtained key(s) in the SSBO for the next compute pass
 */
void computePass(uvec4 key, uint invocation_idx, int active_nodes)
{
    uvec2 nodeID = key.xy;
    uint key_lvl = lt_level_64(nodeID);


    // Check if a merge or division is required
    int parentLod, targetLod;

    if (u_uniform_subdiv > 0) {
         targetLod = parentLod = u_uniform_level;
     } else {
        float parentTargetLevel, targetLevel;
#if FLAG_DISPLACE
        computeTessLvlWithParent(key, cam_height_local, targetLevel, parentTargetLevel);
#else
        computeTessLvlWithParent(key,targetLevel, parentTargetLevel);
#endif
        targetLod = int(targetLevel);
        parentLod = int(parentTargetLevel);
    }

    updateSubdBuffer(key, targetLod, parentLod);
}


////////////////////////////////////////////////////////////////////////////////
///
/// CULL PASS FUNCTIONS
///

// Store the new key in the Culled SSBO for the Render Pass
void cull_writeKey(uvec4 new_key)
{
    uint idx = atomicCounterIncrement(nodeCount_culled[1-u_read_index]);
    u_SubdBufferOut_culled[idx] =  new_key;
}

/**
 * Emulates what was previously the Cull Pass:
 * - Compute the mesh space bounding box of the primitive
 * - Use the BB to check for culling
 * - If not culled, store the (possibly morphed) key in the SSBO for Render
 */
void cullPass(uvec4 key)
{
#if FLAG_CULL
    mat4 mesh_coord;
    vec4 b_min = vec4(10e6);
    vec4 b_max = vec4(-10e6);

    mesh_coord[O] = lt_Leaf_to_MeshPosition(unit_O, key);
    mesh_coord[U] = lt_Leaf_to_MeshPosition(unit_U, key);
    mesh_coord[R] = lt_Leaf_to_MeshPosition(unit_R, key);

#if FLAG_DISPLACE
    mesh_coord[O] = displaceVertex(mesh_coord[O], u_transforms.cam_pos);
    mesh_coord[U] = displaceVertex(mesh_coord[U], u_transforms.cam_pos);
    mesh_coord[R] = displaceVertex(mesh_coord[R], u_transforms.cam_pos);
#endif

    b_min = min(b_min, mesh_coord[O]);
    b_min = min(b_min, mesh_coord[U]);
    b_min = min(b_min, mesh_coord[R]);

    b_max = max(b_max, mesh_coord[O]);
    b_max = max(b_max, mesh_coord[U]);
    b_max = max(b_max, mesh_coord[R]);

    if (culltest(u_transforms.MVP, b_min.xyz, b_max.xyz))
        cull_writeKey(key);
#else
    cull_writeKey(key);
#endif
}

// *********************************** MAIN *********************************** //

void main(void)
{

    uint invocation_idx = int(gl_GlobalInvocationID.x);
    uvec4 key = lt_getKey_64(invocation_idx);

    // Check if the current instance should work
    int active_nodes;

#if FLAG_TRIANGLES
    active_nodes = max(u_num_mesh_tri, int(atomicCounter(nodeCount_full[u_read_index])));
#elif FLAG_QUADS
    active_nodes = max(u_num_mesh_quad * 2, int(atomicCounter(nodeCount_full[u_read_index])));
#endif

    if (invocation_idx >= active_nodes)
        return;

#if FLAG_DISPLACE
    // When subdividing heightfield, we set the plane height to the heightmap
    // value under the camera for more fidelity.
    // To avoid computing the procedural height value in each instance, we
    // store it in a shared variable
    if(gl_LocalInvocationIndex == 0 ){
        cam_height_local = getHeight(u_transforms.cam_pos.xy, u_screen_res);
    }
    barrier();
    memoryBarrierShared();
#endif

    computePass(key, invocation_idx, active_nodes);
    cullPass(key);

    return;
}

#endif
