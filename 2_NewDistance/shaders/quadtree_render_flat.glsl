#line 2
/**
 * In LoD:
 * uniform int poly_type;
 * uniform vec3 cam_pos;
 * uniform mat4 M, V, P, MV, MVP;
 */

const vec4 RED     = vec4(1,0,0,1);
const vec4 GREEN   = vec4(0,1,0,1);
const vec4 BLUE    = vec4(0,0,1,1);
const vec4 CYAN    = vec4(0,1,1,1);
const vec4 MAGENTA = vec4(1,0,0.5,1);
const vec4 YELLOW  = vec4(1,1,0,1);
const vec4 BLACK   = vec4(0,0,0,1);


////////////////////////////////////////////////////////////////////////////////
///
/// VERTEX_SHADER
///

#ifdef VERTEX_SHADER

layout (location = 1) in vec2 tri_p;

layout (location = 0) out Vertex v_vertex;
layout (location = 4) out flat uint v_lvl;
layout (location = 5) out flat uint v_morphed;




layout (binding = LEAF_VERT_B) uniform v_block {
    vec2 positions[100];
};

layout (binding = LEAF_IDX_B) uniform idx_block {
    uint indices[300];
};


#ifndef LOD_GLSL
uniform int morph;
uniform float cpu_lod;
uniform int poly_type;
#endif

#ifndef NOISE_GLSL
uniform float height_factor;
#endif

uniform int heightmap;
uniform int num_vertices, num_indices;

uniform int morph_debug;
uniform float morph_k;

uniform int itpl_type;
uniform float itpl_alpha;

uniform int color_mode;
uniform int render_MVP;


vec3 eye;


// based on Filip Strugar's CDLOD paper (until intPart & signVec)
vec2 morphVertexInUnit(uvec4 key, vec2 leaf_p, vec2 tree_p)
{
    mat3x2 xform;
    lt_getTriangleXform_64(key.xy, xform);
    vec4 mesh_p = M * lt_Leaf_to_MeshPosition(leaf_p, key, false, poly_type);
    float vertex_lvl = distanceToLod(mesh_p.xyz);
    float node_lvl = lt_level_64(key.xy);
    float tessLevel = clamp(node_lvl -  vertex_lvl, 0.0, 1.0);
    float morphK = (morph_debug > 0) ? morph_k : smoothstep(0.4, 0.5, tessLevel);

    if(morphK > 0 && morphK < 1)
        v_morphed = 1;

    // nb of intervals per side of node primitive
    float patchTessFactor = 0x1 << int(cpu_lod);
    vec2 fracPart = fract(leaf_p * patchTessFactor * 0.5) * 2.0 / patchTessFactor;
    vec2 intPart = floor(leaf_p * patchTessFactor * 0.5);
    vec2 signVec = mod(intPart, 2.0) * vec2(-2.0) + vec2(1.0);

    return tree_p - mat2(xform) * (signVec * fracPart) * morphK;
}


vec4 toScreenSpace(vec3 v)
{
    if(render_MVP > 0)
        return MVP * vec4(v.x, v.y, v.z, 1);
    else
        return vec4(v.xyz * 0.2, 1) ;
}

// ------------------------ Main ------------------------ //
void main()
{
    // Reads VAO
    vec2 v_uv = tri_p.xy;

    // Decode key
    uint instanceID = gl_InstanceID;
    uvec4 key = lt_getKey_64(instanceID);
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    uint level = lt_level_64(key.xy);

    v_morphed = 0;

    // Fetch target mesh-space triangle
    Triangle mesh_t;
    lt_getTargetTriangle(poly_type, meshPolygonID, rootID, mesh_t);

    Vertex current_v;

    // Compute Qt position
    vec4 p, n;
    vec2 tree_pos = lt_Leaf_to_Tree_64(v_uv, nodeID, false);
    if (morph > 0)
        tree_pos = morphVertexInUnit(key, v_uv, tree_pos);

    // Interpolates
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

    v_vertex = current_v;
    v_lvl = level;

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

layout(location = 0) out vec4 color;


uniform vec3 light_pos = vec3(0, 110, 00);

const bool flat_n = true;

vec4 levelColor(uint lvl)
{
    vec4 c = vec4(0.0, 0.0, 0.9, 1);
    c.r += (float(lvl) / 10.0);
    if (lvl % 2 == 1) {
        c.g += 0.5;
    }
    c = mix(c, RED, float(v_morphed)*0.5);
    return c;
}

void main()
{
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
        n = normalize(vec3(-s * height_factor,1));
    }
    vec4 n_mv = normalMatrix * vec4(n,0);

    vec4 light_pos_mv =  V * vec4(light_pos, 1.0);
    vec4 l_mv = normalize(light_pos_mv - p_mv);


    float nl =  max(dot(l_mv,n_mv),0.1);
    vec4 c = levelColor(v_lvl);
    color = vec4(c.xyz*nl, 1);

}
#endif

