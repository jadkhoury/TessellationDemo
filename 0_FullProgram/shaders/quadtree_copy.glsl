#line 2

#ifdef COMPUTE_SHADER

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = NODECOUNTER_FULL_B) buffer full_buffer
{
    uint nodeCount_full[16];
};

layout (std430, binding = NODECOUNTER_CULLED_B) buffer culled_buffer
{
    uint nodeCount_culled[16];
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
    uint  primCount;
    uint  first;
    uint  baseVertex;
    uint  baseInstance;
    uvec3  align;
};


uniform int read_index, delete_index;
uniform int num_vertices, num_indices;

void main(void)
{
    uint full_count = nodeCount_full[read_index];
    uint culled_count = nodeCount_culled[read_index];

    //Set the primcount for the draw pass

#ifdef ELEMENTS
    count = num_indices;
    primCount = culled_count;
#else
    count = culled_count * num_indices;
    primCount = 1;
#endif
    //Set the WG siwe for the next compute pass
    workgroup_size_x = uint(full_count / float(LOCAL_WG_COUNT)) + 1;
    // Reset the counters for next round
    nodeCount_full[delete_index] = 0;
    nodeCount_culled[delete_index] = 0;
}

#endif
