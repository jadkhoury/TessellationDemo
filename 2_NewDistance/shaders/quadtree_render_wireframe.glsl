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
uniform int morph;
uniform int cpu_lod;
uniform int poly_type;
#endif

#ifndef NOISE_GLSL
uniform float height_factor;
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
    lt_getTargetTriangle(poly_type, meshPolygonID, rootID, mesh_t);

    Vertex current_v;

    // Compute Qt position
    vec4 p, n;
    vec2 tree_pos = lt_Leaf_to_Tree_64(leaf_pos, nodeID, false);

    // If morphing is activated, morph vertex
    uint morphed = 0;
    if (morph > 0) {
        if (morph_debug > 0) {
            tree_pos = morphVertexDebug(key, leaf_pos, tree_pos, morph_k);
            morphed = 1;
        } else {
            tree_pos = morphVertex(key, leaf_pos, tree_pos, morphed);
        }
    }

    // Interpolate
    switch(itpl_type) {
    case LINEAR:
        current_v = lt_interpolateVertex(mesh_t, tree_pos);
        break;
    case PN:
        PNInterpolation(mesh_t, tree_pos, poly_type, itpl_alpha, current_v);
        break;
    case PHONG:
        PhongInterpolation(mesh_t, tree_pos, poly_type, itpl_alpha, current_v);
        break;
    }

    if (heightmap > 0)
        current_v.p.xyz =  displaceVertex(current_v.p.xyz, cam_pos);

    // Pass relevant values
    v_vertex = current_v;
    v_lvl = level;
    v_leaf_pos = leaf_pos;
    v_morphed = morphed;
}
#endif

////////////////////////////////////////////////////////////////////////////////
///
/// GEOMETRY_SHADER
///

#ifdef GEOMETRY_SHADER


layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in Vertex v_vertex[];
layout (location = 4) in flat uint v_lvl[];
layout (location = 5) in flat uint v_morphed[];
layout (location = 6) in vec2 v_leaf_pos[];

layout (location = 0) out Vertex g_vertex;
layout (location = 4) out flat uint g_lvl;
layout (location = 5) out flat uint g_morphed;
layout (location = 6) out vec2 g_leaf_pos;
layout (location = 7) out vec2 g_tri_pos;

void main()
{
    for (int i = 0; i < gl_in.length(); i++) {
        //Passthrough
        g_vertex = v_vertex[i];
        g_lvl = v_lvl[i];
        g_morphed = v_morphed[i];
        g_leaf_pos = v_leaf_pos[i];
        // Triangle pos for solid wireframe
        g_tri_pos = vec2(i>>1, i & 1u);

        // Final screen position
        gl_Position = toScreenSpace(g_vertex.p.xyz);
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

layout (location = 0) in Vertex g_vertex;
layout (location = 4) in flat uint g_lvl;
layout (location = 5) in flat uint g_morphed;
layout (location = 6) in vec2 g_leaf_pos;
layout (location = 7) in vec2 g_tri_pos;

layout(location = 0) out vec4 color;

uniform vec3 light_pos = vec3(0, 0, 100);

const bool flat_n = true;

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
    vec3 p = g_vertex.p.xyz;
    vec4 p_mv = MV * g_vertex.p;
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
        n = normalize(vec3(-s * height_factor,1));
    }
    vec4 n_mv = normalMatrix * vec4(n,0);

    vec4 light_pos_mv =  V * vec4(light_pos, 1.0);
    vec4 l_mv = normalize(light_pos_mv - p_mv);

    float nl =  max(dot(l_mv,n_mv),0.1);
    vec4 c = levelColor(g_lvl, g_morphed);

    float wireframe_factor = gridFactor(g_tri_pos, 1.0);

    color = vec4(c.xyz*wireframe_factor, 1);

}
#endif

