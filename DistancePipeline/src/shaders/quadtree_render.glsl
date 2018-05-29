#line 2
/* ---- In ltree_common.glsl ----- *
 struct dj_vertex;

 uniform mat3 shear_mat;
 uniform float adaptive_factor;
 uniform int prim_type;
 uniform vec3 cam_pos;

 mat3 toLocal, toWorld;
 dj_vertex mesh_v[];
 uint mesh_q_idx[];
 uint  mesh_t_idx[];

 float distanceToLod(float x);
 void getMeshMapping(uint idx, out mat3 toUnit, out mat3 toOriginal);
*/
uniform int render_MVP;
uniform mat4 M, V, P, MVP;

vec4 toScreenSpace(vec3 v)
{
    if (render_MVP > 0)
        return MVP * vec4(v.x, v.y, v.z, 1);
    else
        return vec4(v.xyz*0.35, 1);
}

#ifdef VERTEX_SHADER

layout (location = 0) in vec2 quad_p;
layout (location = 1) in vec2 tri_p;

layout (std430, binding = 0) readonly buffer Data_In
{
    uvec4 tree_nodes[];
};


layout (location = 0) out vec4 v_color;
layout (location = 1) out vec3 v_pos;
layout (location = 3) out vec2 v_uv;


uniform int morph;
uniform int debug_morph;
uniform float morph_k;
uniform int heightmap;
uniform int cpu_lod;

vec4 levelColor(uvec2 key)
{
    uint lvl = lt_level_2_30(key);
    vec4 c = vec4(0.0, 0.0, 0.9, 1);
    c.r += (float(lvl) / 10.0);
    if(lvl % 2 == 1){
        c.g += 0.5;
    }
    return c;
}

// based on Filip Strugar's CDLOD paper (until intPart & signVec)
vec2 morphVertex(vec2 gridPos, vec2 vertex, float morphK, in mat3 test)
{

    if(debug_morph > 0)
        morphK = morph_k;
    mat2 t = mat2(test);
    float patchTessFactor = 0x1 << cpu_lod; // = nb of intervals per side of node primitive
    vec2 fracPart = fract(gridPos.xy * patchTessFactor * 0.5) * 2.0 / patchTessFactor;
    vec2 intPart = floor(gridPos.xy * patchTessFactor * 0.5);
    vec2 signVec = mod(intPart, 2.0) * vec2(2.0) - vec2(1.0);
    if(patchTessFactor == 2)
        signVec.x *= -1;
#if 1
    signVec = -signVec;
#endif
    return vertex.xy - t * (signVec * fracPart) * morphK;
}



uint getKey(uint idx)
{
    return tree_nodes[idx][0];
}

uvec2 getKey30(uint idx)
{
    return tree_nodes[idx].xy;
}

void main()
{
    uvec2 key = getKey30(gl_InstanceID);
    mat3 xform;
    vec3 node_v_pos, pos_3d;
    vec2 prim_v_pos;
    Quad q;
    Triangle t;

    if(prim_type == QUADS)
    {
        lt_get_quad_xform_2_30(key, xform);
        prim_v_pos = quad_p.xy;
        getMeshQuad(tree_nodes[gl_InstanceID][2], q);
    }
    else if(prim_type == TRIANGLES)
    {
        lt_get_triangle_xform_2_30(key, xform);
        prim_v_pos = tri_p;
        getMeshTriangle(tree_nodes[gl_InstanceID][2], t);
    }
    node_v_pos = xform * vec3(prim_v_pos, 1.0);
    node_v_pos /= node_v_pos.z;

    if(morph > 0 && cpu_lod > 0)
    {
        // Theoretical LoD of the vertex
        vec3 pos_w;
        if (prim_type == QUADS) {
            pos_w = vec3(M * mapToMeshQuad(q, node_v_pos.xy));
        } else if (prim_type == TRIANGLES) {
            pos_w = vec3(M * mapToMeshTriangle(t, node_v_pos.xy));
        }
        float z = distance(cam_pos, pos_w);
        float vertex_lvl = distanceToLod(z);
        // LoD of the patch
        float node_lvl = lt_level_2_30(key);
        // Morph Factor in function of those LoD
        float tessLevel = clamp(node_lvl -  vertex_lvl, 0.0, 1.0);
        tessLevel = smoothstep(0.4, 0.5, tessLevel);
        // Morphing position
        node_v_pos = vec3(morphVertex(prim_v_pos, node_v_pos.xy, tessLevel, xform), 1.0);
    }
    // Projecting position to world space
    if (prim_type == QUADS) {
        pos_3d = mapToMeshQuad(q, node_v_pos.xy).xyz;
    } else if (prim_type == TRIANGLES) {
        pos_3d = mapToMeshTriangle(t, node_v_pos.xy).xyz;
    }

    if(heightmap > 0)
        pos_3d.z =  displace(pos_3d, 1000) * 2.0;
    v_pos = pos_3d;
    v_color = levelColor(key);
    v_uv = prim_v_pos.xy;

}
#endif


#ifdef GEOMETRY_SHADER

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec4 v_color[];
layout (location = 1) in vec3 v_pos[];
layout (location = 3) in vec2 v_uv[];

layout (location = 0) out vec4 g_color;
layout (location = 1) out vec3 g_pos;
layout (location = 2) out vec3 g_pos3D;
layout (location = 3) out vec2 g_leaf_uv;
layout (location = 4) out vec2 g_root_uv;

void main ()
{
    for (int i = 0; i < gl_in.length(); i++){
        g_color = v_color[i];
        g_pos = v_pos[i];
        g_pos3D = vec3(g_pos);
        g_leaf_uv = vec2(i>>1, i & 0x1);
        g_root_uv = v_uv[i];
        gl_Position = toScreenSpace(g_pos3D);
        EmitVertex();
    }
    EndPrimitive();
}
#endif


#ifdef FRAGMENT_SHADER

layout (location = 0) in vec4 g_color;
layout (location = 1) in vec3 g_pos;
layout (location = 2) in vec3 g_pos3D;
layout (location = 3) in vec2 g_leaf_uv;
layout (location = 4) in vec2 g_root_uv;

layout(location = 0) out vec4 color;

uniform int color_mode;

// https://github.com/rreusser/glsl-solid-wireframe
float gridFactor (vec2 vBC, float width) {
    vec3 bary = vec3(vBC.x, vBC.y, 1.0 - vBC.x - vBC.y);
    vec3 d = fwidth(bary);
    vec3 a3 = smoothstep(d * (width - 0.5), d * (width + 0.5), bary);
    return min(min(a3.x, a3.y), a3.z);
}

void main()
{
    float h;
    //vec3 n = normalize(rock_normal(g_pos, 10000, h));
    //color.rgb = 3.0 * irradiance(n) * albedo(g_pos,n);

    vec3 white_leaves = vec3(gridFactor(g_leaf_uv, 0.5));
    if (color_mode == WHITE) {
        color = vec4(white_leaves, 1);
    } else if (color_mode == LOD){
        color = vec4(white_leaves, 1.0) * g_color;
    } else if (color_mode == PRIMITIVES) {
        vec3 red_roots = vec3(1 - gridFactor(g_root_uv, 1.0), 0.0, 0.0);
        if(red_roots != vec3(0.0))
            color = vec4(red_roots, 1);
        else
            color = vec4(white_leaves * 0.5, 1);
    }
}
#endif

