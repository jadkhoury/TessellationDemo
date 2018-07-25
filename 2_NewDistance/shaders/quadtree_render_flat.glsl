#line 2
////////////////////////////////////////////////////////////////////////////////
///
/// VERTEX_SHADER
///

#ifdef VERTEX_SHADER

layout (location = 1) in vec2 tri_p;

layout (location = 0) out Vertex v_vertex;
layout (location = 4) out flat uint v_lvl;
layout (location = 5) out flat uint v_morphed;
layout (location = 6) out vec2 v_leaf_pos;

layout (binding = LEAF_VERT_B) uniform v_block {
    vec2 positions[100];
};

layout (binding = LEAF_IDX_B) uniform idx_block {
    uint indices[300];
};

#ifndef LOD_GLSL
uniform int u_morph_on;
uniform int u_cpu_lod;
uniform int u_poly_type;
#endif

#ifndef NOISE_GLSL
uniform float u_displace_factor;
#endif

vec3 eye;

// ------------------------ Main ------------------------ //
void main()
{

    // Read VAO
    vec2 leaf_pos = tri_p.xy;

    // Decode key
    uint instanceID = gl_InstanceID;
    uvec4 key = lt_getKey_64(instanceID);
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 3u;
    uint level = lt_level_64(key.xy);

    v_morphed = 0;

    // Fetch target mesh-space triangle
    Triangle mesh_t;
    lt_getTargetTriangle(u_poly_type, meshPolygonID, rootID, mesh_t);

    Vertex current_v;

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
    switch(u_itpl_type) {
    case LINEAR:
        current_v = lt_interpolateVertex(mesh_t, tree_pos);
        break;
    case PN:
        PNInterpolation(mesh_t, tree_pos, u_poly_type, u_itpl_alpha, current_v);
        break;
    case PHONG:
        PhongInterpolation(mesh_t, tree_pos, u_poly_type, u_itpl_alpha, current_v);
        break;
    }

    if (u_displace_on > 0)
        current_v.p.xyz =  displaceVertex(current_v.p.xyz, cam_pos);

    // Pass relevant values
    v_vertex = current_v;
    v_lvl = level;
    v_leaf_pos = leaf_pos;
    v_morphed = morphed;

    gl_Position = toScreenSpace(current_v.p.xyz);
}
#endif

////////////////////////////////////////////////////////////////////////////////
///
/// FRAGMENT_SHADER
///

#ifdef FRAGMENT_SHADER

layout (location = 0) in Vertex v_vertex;
layout (location = 4) in flat uint v_lvl;
layout (location = 5) in flat uint v_morphed;
layout (location = 6) in vec2 v_leaf_pos;

layout(location = 0) out vec4 color;


uniform vec3 u_light_pos = vec3(0, 110, 00);

const bool flat_n = true;

void main()
{
#if 1
    // Position
    vec3 p = v_vertex.p.xyz;
    vec4 p_mv = MV * v_vertex.p;
    //Normal
    vec3 n;
    mat4 normalMatrix = transpose(inverse(MV));
    vec3 dx = dFdx(p), dy = dFdy(p);
    if (flat_n){
        n = normalize(cross(dx,dy));
    } else {
        float dp = sqrt(dot(dx,dx));
        vec2 s;
        float d = displace(p.xy, 1.0/(0.5*dp), s);
        n = normalize(vec3(-s * u_displace_factor,1));
    }
    vec4 n_mv = normalMatrix * vec4(n,0);

    vec4 light_pos_mv =  V * vec4(u_light_pos, 1.0);
    vec4 l_mv = normalize(light_pos_mv - p_mv);

    float nl =  max(dot(l_mv,n_mv),0.1);
    vec4 c = levelColor(v_lvl, v_morphed);
    color = vec4(c.xyz*nl, 1);
#else
    color = RED;
#endif
}
#endif

