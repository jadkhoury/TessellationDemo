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

uniform int color_mode;
uniform int render_MVP;

uniform float cpu_lod;

////////////////////////////////////////////////////////////////////////////////
///
/// VERTEX_SHADER
///

#ifdef VERTEX_SHADER

layout (location = 1) in vec2 tri_p;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec3 v_pos;
layout (location = 2) out vec2 v_uv;

layout (binding = LEAF_VERT_B) uniform v_block {
    vec2 positions[100];
};

layout (binding = LEAF_IDX_B) uniform idx_block {
    uint indices[300];
};


uniform int morph;
uniform int heightmap;
uniform int num_vertices, num_indices;

uniform int morph_debug;
uniform float morph_k;


uniform int ipl_on;
uniform float ipl_alpha;

// ------------------- Color Functions ------------------- //

vec4 levelColor(uint lvl)
{
    vec4 c = vec4(0.0, 0.0, 0.9, 1);
    c.r += (float(lvl) / 10.0);
    if (lvl % 2 == 1) {
        c.g += 0.5;
    }
    return c;
}


vec4 rootColor(in vec2 pos, in vec4 color)
{
    if(pos.x < 0.01 || pos.y < 0.01 || abs(1.0 - (pos.x + pos.y)) < 0.01)
        return vec4(1);
    else
        return color;
}

vec4 distanceColor(vec3 p)
{
    float d = closestPaneDistance(MVP, p) * 3;
    vec4 color = BLUE;
    if(abs(d) < 0.01)
        color = vec4(1);
    else if (d < 0)
        color = mix(BLUE, RED, -d);
    else
        color = mix(BLUE, CYAN, d);
    return color;
}

vec4 cullColor(vec3 p)
{
    if (culltest(MVP, p, p))
        return GREEN;
    else
        return RED;
}

vec4 diffuseColor(vec3 p_mv, vec3 n_mv, vec3 light_dir)
{
    vec3 ka = BLUE.xyz * 2.0;
    vec3 kd = CYAN.xyz ;

    vec3 La = vec3(0.2);
    vec3 Ld = vec3(0.8);

    vec3 n = normalize(n_mv);
    vec3 l = normalize(light_dir);

    vec3 ambiant = ka*La;

    float nl = max(0.0, dot(n,l));
    vec3 diffuse =  kd * nl * Ld;

    vec3 c = ambiant + diffuse;
    return vec4(c,1);
}


// ------------------- Geometry Functions ------------------- //

// based on Filip Strugar's CDLOD paper (until intPart & signVec)
vec2 morphVertex(uvec4 key, vec2 leaf_p, vec2 tree_p)
{
    mat3 t;
    lt_getTriangleXform_64(key.xy, t);
    vec4 mesh_p = M * lt_Leaf_to_MeshPrimitive(leaf_p, key, false, poly_type);
    float vertex_lvl = distanceToLod(mesh_p.xyz);


    float node_lvl = lt_level_64(key.xy);
    float tessLevel = clamp(node_lvl -  vertex_lvl, 0.0, 1.0);
    float morphK = (morph_debug > 0) ? morph_k : smoothstep(0.4, 0.5, tessLevel);

    float patchTessFactor = 1u << uint(cpu_lod); // = nb of intervals per side of node primitive
    vec2 fracPart = fract(leaf_p * patchTessFactor * 0.5) * 2.0 / patchTessFactor;
    vec2 intPart = floor(leaf_p * patchTessFactor * 0.5);
    vec2 signVec = mod(intPart, 2.0) * vec2(2.0) - vec2(1.0);
    if(patchTessFactor == 2)
        signVec.x *= -1;
    return tree_p - mat2(t) * (-1*signVec * fracPart) * morphK;
}

vec3 heightDisplace(in vec3 v, out vec3 n) {
    vec3 r = v;
    r.z = displace(r, 1000, n) * 2.0;
    return r;
}

// ------------------------ Main ------------------------ //
void main()
{
    // Recover information about the current vertex
    v_uv = tri_p.xy;

    // Recover data about the current quadtree node
    uint instanceID = gl_InstanceID;
    uvec4 key = lt_getKey_64(instanceID);
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 3u;
    uint level = lt_level_64(key.xy);



    vec4 p, n;
    if(ipl_on == 0) {
        // Compute mesh-space position after morphing
        v_pos = lt_Leaf_to_MeshPrimitive(v_uv, key, false, poly_type).xyz;
        vec2 tree_pos = lt_Leaf_to_Tree_64(v_uv, nodeID, false);
        if (morph > 0)
            tree_pos = morphVertex(key, v_uv, tree_pos);
        v_pos = lt_Tree_to_MeshTriangle(tree_pos, poly_type, key.z, key.w).xyz;

        // Update normal and position for displacement mapping
        if (heightmap > 0) {
            n = vec4(0,0,1,0);
            v_pos =  heightDisplace(v_pos, n.xyz);
        } else {
            n = lt_getMeanPrimNormal(key, poly_type);
        }
    } else {
        vec2 tree_pos = lt_Leaf_to_Tree_64(v_uv, nodeID, false);
        p = vec4(0.0);
        n = vec4(0.0);
        if (morph > 0)
            tree_pos = morphVertex(key, v_uv, tree_pos);
        PNInterpolation(key, tree_pos, poly_type, ipl_alpha, p, n);
        v_pos = p.xyz;
    }

    // Assign color to current vertex
    switch (color_mode) {
    case LOD:
        v_color = levelColor(level);
        break;
    case FRUSTUM:
        v_color = distanceColor(v_pos);
        break;
    case CULL:
        v_color = cullColor(v_pos);
        break;
    case DEBUG:
#if 0
        vec3 normal_mv = vec3(invMV * n);
        vec3 v_pos_mv =  vec3(MV * vec4(v_pos,1));
        vec3 light_pos = vec3(V * vec4(10,0,0,1));
        vec3 light_dir = light_pos - v_pos_mv;
        v_color = diffuseColor(v_pos_mv, normal_mv, light_dir);
#else
        v_color = vec4(n.xyz*0.5+0.5, 1.0);
        if(key.w == 0)
            v_color = RED * 0.5 + 0.5;
        else
            v_color = BLUE * 0.5 + 0.5;
#endif
        break;
    default:
        v_color = vec4(1);
        break;
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////
///
/// GEOMETRY_SHADER
///

#ifdef GEOMETRY_SHADER

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec4 v_color[];
layout (location = 1) in vec3 v_pos[];
layout (location = 2) in vec2 v_uv[];

layout (location = 0) out vec4 g_color;
layout (location = 1) out vec3 g_pos;
layout (location = 2) out vec3 g_pos3D;
layout (location = 3) out vec2 g_leaf_uv;
layout (location = 4) out vec2 g_root_uv;

vec4 toScreenSpace(vec3 v)
{
    if(render_MVP > 0)
        return MVP * vec4(v.x, v.y, v.z, 1);
    else
        return vec4(v.xyz * 0.2, 1) ;
}

void main()
{
    for (int i = 0; i < gl_in.length(); i++) {
        g_color = v_color[i];
        g_pos = v_pos[i];
        g_pos3D = g_pos;
        g_leaf_uv = vec2(i>>1, i & 1u);
        g_root_uv = v_uv[i];
        gl_Position = toScreenSpace(g_pos3D);
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
layout (location = 0) in vec4 g_color;
layout (location = 1) in vec3 g_pos;
layout (location = 2) in vec3 g_pos3D;
layout (location = 3) in vec2 g_leaf_uv;
layout (location = 4) in vec2 g_root_uv;

layout(location = 0) out vec4 color;


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

    vec4 white_leaves = vec4(vec3(gridFactor(g_leaf_uv, 0.5)), 1);
    switch (color_mode)
    {
    case WHITE_WIREFRAME:
        color = white_leaves;
        break;
    case PRIMITIVES:
        vec4 poly_outline = vec4(vec3(gridFactor(g_root_uv, 1.5)), 1);
        color = (poly_outline.r == 0) ? poly_outline : white_leaves;
        break;
    case LOD:
    case FRUSTUM:
    case CULL:
    case DEBUG:
        color = (vec4(1,1,1,2) - white_leaves) * g_color;
        break;
    default:
        color = white_leaves;
    }
}
#endif

