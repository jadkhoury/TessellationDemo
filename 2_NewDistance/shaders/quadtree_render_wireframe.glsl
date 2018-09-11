#line 2
////////////////////////////////////////////////////////////////////////////////
///
/// VERTEX_SHADER
///

#ifdef VERTEX_SHADER

layout (location = 1) in vec2 i_vpos;

layout (location = 0) out flat uint o_lvl;
layout (location = 1) out flat uint o_morphed;
layout (location = 2) out vec2 o_leaf_pos;
layout (location = 3) out Vertex o_vertex;

#ifndef LOD_GLSL
uniform int u_morph_on;
uniform int u_cpu_lod;
#endif

#ifndef NOISE_GLSL
uniform float u_displace_factor;
#endif

// ------------------------ Main ------------------------ //
void main()
{
    // Read VAO
    vec2 leaf_pos = i_vpos.xy;

    // Decode key
    uint instanceID = gl_InstanceID;
    uint primID = u_SubdBufferIn[instanceID].x;
    uint key    = u_SubdBufferIn[instanceID].y;

    int keyLod = findMSB(key);
    uint morphed = 0;
    // Fetch target mesh-space triangle


    // Map from leaf to quadtree position
    mat3 xf = keyToXform(key);
    vec2 tree_pos = vec2(xf * vec3(i_vpos, 1.0));
#if 0

    vec3 t[3];
    getMeshTriangle(primID, t);

    // Interpolate
    Vertex current_v;
    current_v.p.xyz = berp(t, tree_pos);

#else

    vec3 v_in[3];
    v_in[0] = u_MeshVertex[u_TriangleIdx[primID*3 + 0]].p.xyz;
    v_in[1] = u_MeshVertex[u_TriangleIdx[primID*3 + 1]].p.xyz;
    v_in[2] = u_MeshVertex[u_TriangleIdx[primID*3 + 2]].p.xyz;

    Vertex current_v;
    current_v.p.xyz = berp(v_in, tree_pos);

#endif

    // Pass relevant values
    o_vertex = current_v;
    o_lvl = keyLod;
    o_leaf_pos = leaf_pos;
    o_morphed = morphed;
}
#endif

////////////////////////////////////////////////////////////////////////////////
///
/// GEOMETRY_SHADER
///

#ifdef GEOMETRY_SHADER


layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in flat uint i_lvl[];
layout (location = 1) in flat uint i_morphed[];
layout (location = 2) in vec2 i_leaf_pos[];
layout (location = 3) in Vertex i_vertex[];

layout (location = 0) out flat uint o_lvl;
layout (location = 1) out flat uint o_morphed;
layout (location = 2) out vec2 o_leaf_pos;
layout (location = 3) out vec2 o_tri_pos;
layout (location = 4) out Vertex o_vertex;

void main()
{
    for (int i = 0; i < gl_in.length(); i++) {
        //Passthrough
        o_vertex = i_vertex[i];
        o_lvl = i_lvl[i];
        o_morphed = i_morphed[i];
        o_leaf_pos = i_leaf_pos[i];
        // Triangle pos for solid wireframe
        o_tri_pos = vec2(i>>1, i & 1u);

        // Final screen position
        gl_Position = toScreenSpace(o_vertex.p.xyz);
        EmitVertex();
    }
    EndPrimitive();
}
#endif

////////////////////////////////////////////////////////////////////////////////
///
/// FRAGMENT_SHADER
///

#ifdef FRAGMENT_SHADER

layout (location = 0) in flat uint i_lvl;
layout (location = 1) in flat uint i_morphed;
layout (location = 2) in vec2 i_leaf_pos;
layout (location = 3) in vec2 i_tri_pos;
layout (location = 4) in Vertex i_vertex;

layout(location = 0) out vec4 o_color;

// https://github.com/rreusser/glsl-solid-wireframe
float gridFactor (vec2 vBC, float width) {
    vec3 bary = vec3(vBC.x, vBC.y, 1.0 - vBC.x - vBC.y);
    vec3 d = fwidth(bary);
    vec3 a3 = smoothstep(d * (width - 0.5), d * (width + 0.5), bary);
    return min(min(a3.x, a3.y), a3.z);
}

void main()
{
    // Position
    vec3 p = i_vertex.p.xyz;
    vec4 p_mv = MV * i_vertex.p;

    vec4 c = levelColor(i_lvl, i_morphed);

    float wireframe_factor = gridFactor(i_tri_pos, 1.0);

    o_color = vec4(c.xyz*wireframe_factor, 1);

}
#endif

