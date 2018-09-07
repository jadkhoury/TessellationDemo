#line 2

#ifdef COMPUTE_SHADER

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = NODECOUNTER_FULL_B) buffer full_buffer
{
    uint nodeCount_full[2];
};

layout (std430, binding = DISPATCH_COUNTER_B) buffer dispatch_out
{
    uint workgroup_size_x;
    uint workgroup_size_y;
    uint workgroup_size_z;
};

layout (std430, binding = DRAW_INDIRECT_B) buffer draw_out
{
    uint  count;
    uint  nodeCount;
    uint  first;
    uint  baseVertex;
    uint  baseInstance;
    uvec3  align;
};


uniform int u_read_index;
uniform int u_num_vertices, u_num_indices;

void main(void)
{
    uint full_count = nodeCount_full[u_read_index];

    //Set the nodeCount for the draw pass
    count = u_num_indices;
    nodeCount = full_count;
    //Set the WG siwe for the next compute pass
    workgroup_size_x = uint(full_count / float(LOCAL_WG_COUNT)) + 1;
    // Reset the counters for next round
    nodeCount_full[1-u_read_index] = 0;
}

#endif
