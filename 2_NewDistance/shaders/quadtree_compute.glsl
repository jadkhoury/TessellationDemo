#line 2

#ifdef COMPUTE_SHADER

layout (local_size_x = LOCAL_WG_SIZE_X,
        local_size_y = LOCAL_WG_SIZE_Y,
        local_size_z = LOCAL_WG_SIZE_Z) in;

layout (binding = NODECOUNTER_FULL_B)   uniform atomic_uint nodeCount_full[2];


uniform int u_read_index;

uniform int u_uniform_subdiv;
uniform int u_uniform_level;

uniform int u_num_mesh_tri;
uniform int u_num_mesh_quad;
uniform int u_max_node_count;

uniform int u_cull;

#ifndef LOD_GLSL
uniform int u_screen_res;
uniform int u_mode;
#endif

uniform int u_displace_on;

vec3 eye;

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

void compute_writeKey(uint primID, uint key)
{
    uint idx = atomicCounterIncrement(nodeCount_full[1-u_read_index]);
    u_SubdBufferOut[idx] = uvec2(primID, key);
}


float computeLod(vec3 c)
{
    vec3 cxf = (MV * vec4(c, 1)).xyz;
    float z = length(cxf);

    return distanceToLod(z);
}

float computeLod(in vec3 v[3])
{
    vec3 c = (v[2] + v[1]) / 2.0;
    return computeLod(c);
}

/**
 * Writes the keys in the buffer as dictated by the merge / split operators
 */
void updateSubdBuffer(uint primID, uint key, int targetLod, int parentLod)
{
    // extract subdivision level associated to the key
    int keyLod = findMSB(key);

    // update the key accordingly
    if (/* subdivide ? */ keyLod < targetLod && !isLeafKey(key)) {
        uint children[2]; childrenKeys(key, children);
        compute_writeKey(primID, children[0]);
        compute_writeKey(primID, children[1]);
    } else if (/* keep ? */ keyLod <= (parentLod + 1)) {
        compute_writeKey(primID, key);
    } else /* merge ? */ {
        if (/* is root ? */isRootKey(key)) {
            compute_writeKey(primID, key);
        } else if (/* is zero child ? */isChildZeroKey(key)) {
            compute_writeKey(primID, parentKey(key));
        }
    }
}

// *********************************** MAIN *********************************** //

void main(void)
{

    uint invocation_idx = int(gl_GlobalInvocationID.x);
    uint primID = u_SubdBufferIn[invocation_idx].x;
    uint key    = u_SubdBufferIn[invocation_idx].y;

    // Check if the current instance should work
    int active_nodes;

#ifdef TRIANGLES
    active_nodes = max(u_num_mesh_tri, int(atomicCounter(nodeCount_full[u_read_index])));
#elif defined(QUADS)
    active_nodes = max(u_num_mesh_quad * 2, int(atomicCounter(nodeCount_full[u_read_index])));
#endif

    if (invocation_idx >= active_nodes)
        return;

    int parentLod, targetLod;
    if (u_uniform_subdiv > 0) {
        targetLod = u_uniform_level;
        parentLod = u_uniform_level - 1;
    } else {
        vec3 v_in[3];
        vec3 v[3], vp[3];

        v_in[0] = u_MeshVertex[u_TriangleIdx[primID*3 + 0]].p.xyz;
        v_in[1] = u_MeshVertex[u_TriangleIdx[primID*3 + 1]].p.xyz;
        v_in[2] = u_MeshVertex[u_TriangleIdx[primID*3 + 2]].p.xyz;

        subd(key, v_in, v, vp);
        targetLod = int(computeLod(v));
        parentLod = int(computeLod(vp));
    }
    updateSubdBuffer(primID, key, targetLod, parentLod);

    return;
}

#endif
