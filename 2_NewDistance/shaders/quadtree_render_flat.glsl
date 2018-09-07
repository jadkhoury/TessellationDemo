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
    uvec4 key = u_SubdBufferIn[instanceID];
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 3u;
    uint key_lod = lt_level_64(key.xy);

    // Fetch target mesh-space triangle
    Triangle mesh_t;
    lt_getTargetTriangle(meshPolygonID, rootID, mesh_t);

    // Perform T-Junction Removal
    uint morphed = 0;

    // Map from leaf to quadtree position
    vec2 tree_pos = lt_Leaf_to_Tree_64(leaf_pos, nodeID, false);

    // Interpolate
    Vertex current_v = interpolate(mesh_t, tree_pos, u_itpl_alpha);

    if (u_displace_on > 0)
        current_v.p.xyz =  displaceVertex(current_v.p.xyz, cam_pos);

    // Pass relevant values
    o_vertex = current_v;
    o_lvl = key_lod;
    o_leaf_pos = leaf_pos;
    o_morphed = morphed;

    gl_Position = toScreenSpace(current_v.p.xyz);
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
layout (location = 3) in Vertex i_vertex;

layout(location = 0) out vec4 o_color;


uniform vec3 u_light_pos = vec3(0, 110, 00);

const bool flat_n = true;

void main()
{
    // Position
    vec3 p = i_vertex.p.xyz;
    vec4 p_mv = MV * i_vertex.p;
    //Normal
    vec3 n;
    vec3 dx = dFdx(p), dy = dFdy(p);
    if (flat_n){
        n = normalize(cross(dx,dy));
    } else {
        float dp = sqrt(dot(dx,dx));
        vec2 s;
        float d = displace(p.xy, 1.0/(0.5*dp), s);
        n = normalize(vec3(-s * u_displace_factor,1));
    }
    vec4 n_mv = invMV * vec4(n,0);

    vec4 light_pos_mv =  V * vec4(u_light_pos, 1);
    vec4 l_mv = normalize(light_pos_mv - p_mv);

    float nl =  max(dot(l_mv,n_mv), 0);
    vec4 c = levelColor(i_lvl, i_morphed);
    o_color = vec4(c.xyz*nl, 1);
}
#endif

