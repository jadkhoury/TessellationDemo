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
uniform int u_poly_type;
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
    uvec4 key = lt_getKey_64(instanceID);
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 3u;
    uint level = lt_level_64(key.xy);

    // Fetch target mesh-space triangle
    Triangle mesh_t;
    lt_getTargetTriangle(u_poly_type, meshPolygonID, rootID, mesh_t);


    // Compute Qt position
    vec4 p, n;
    vec2 tree_pos = lt_Leaf_to_Tree_64(leaf_pos, nodeID, false);

    // If morphing is activated, u_morph_on vertex
    uint morphed = 0;
    if (u_morph_on > 0) {
        if (u_morph_debug > 0) {
            tree_pos = morphVertexDebug(key, leaf_pos, tree_pos, u_morph_k);
            morphed = 1;
        } else {
            tree_pos = morphVertex(key, leaf_pos, tree_pos, morphed);
        }
    }

    // Interpolate
    // Interpolate
    Vertex current_v = interpolate(mesh_t, tree_pos, u_poly_type, u_itpl_alpha);


    if (u_displace_on > 0)
        current_v.p.xyz =  displaceVertex(current_v.p.xyz, cam_pos);

    // Pass relevant values
    o_vertex = current_v;
    o_lvl = level;
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

