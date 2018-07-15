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
layout (location = 2) out Vertex vertex;



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

vec4 UvColor(vec2 uv)
{
    return vec4(uv.x, uv.y, 0.2, 1.0);
}

// ------------------- Geometry Functions ------------------- //

// based on Filip Strugar's CDLOD paper (until intPart & signVec)
vec2 morphVertexInUnit(uvec4 key, vec2 leaf_p, vec2 tree_p)
{
    mat3x2 xform;
    lt_getTriangleXform_64(key.xy, xform);
    vec4 mesh_p = M * lt_Leaf_to_MeshPrimitive(leaf_p, key, false, poly_type);
    float vertex_lvl = distanceToLod(mesh_p.xyz);


    float node_lvl = lt_level_64(key.xy);
    float tessLevel = clamp(node_lvl -  vertex_lvl, 0.0, 1.0);
    float morphK = (morph_debug > 0) ? morph_k : smoothstep(0.4, 0.5, tessLevel);

    float patchTessFactor = 0x1 << int(cpu_lod); // = nb of intervals per side of node primitive
    vec2 fracPart = fract(leaf_p * patchTessFactor * 0.5) * 2.0 / patchTessFactor;
    vec2 intPart = floor(leaf_p * patchTessFactor * 0.5);
    vec2 signVec = mod(intPart, 2.0) * vec2(2.0) - vec2(1.0);
    if(patchTessFactor == 2)
        signVec.x *= -1;
    return tree_p - mat2(xform) * (-1*signVec * fracPart) * morphK;
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
    // Recover information about the current vertex
    vec2 v_uv = tri_p.xy;

    // Recover data about the current quadtree node
    uint instanceID = gl_InstanceID;
    uvec4 key = lt_getKey_64(instanceID);
    uvec2 nodeID = key.xy;
    uint meshPolygonID = key.z;
    uint rootID = key.w & 0x3;
    uint level = lt_level_64(key.xy);


    vec4 p, n;
    vec2 tree_pos = lt_Leaf_to_Tree_64(v_uv, nodeID, false);
    if (morph > 0)
        tree_pos = morphVertexInUnit(key, v_uv, tree_pos);

    if(ipl_on == 0)
    {
        // Compute mesh-space position after morphing
        vertex = lt_Tree_to_MeshTriangleVertex(tree_pos, poly_type, key.z, key.w);
        // Update normal and position for displacement mapping
        if (heightmap > 0)
            vertex.p.xyz =  displaceVertex(vertex.p.xyz, cam_pos);
    }
    else
    {
        PNInterpolation(key, tree_pos, poly_type, ipl_alpha, vertex);
    }

    gl_Position = toScreenSpace(vertex.p.xyz);
}
#endif

////////////////////////////////////////////////////////////////////////////////
///
/// FRAGMENT_SHADER
///

#ifdef FRAGMENT_SHADER
layout (location = 0) in vec4 v_color;
layout (location = 2) in Vertex vertex;



layout(location = 0) out vec4 color;

const bool flat_n = true;
const vec3 light_pos = vec3(0, 50, 100);

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(MV)));
    vec3 n, p = vertex.p.xyz;
    float depth = (MVP * vec4(p, 1.0)).z;
    vec3 dx = dFdx(p.xyz);
    vec3 dy = dFdy(p.xyz);
    if (flat_n){
        n = normalize(cross(dx,dy));
    } else {
        float dp = sqrt(dot(dx,dx));
        vec2 s;
        float d = displace(p.xy, 1.0/(0.5*dp), s);
        n = normalize(vec3(-s * height_factor,1));
    }

    vec3 l = normalize(light_pos - vertex.p.xyz);

    vec3 c = vec3(0.5) * max(dot(l,n),0.0);

    color = vec4(vertex.uv * max(dot(l,n),0.0), 0, 1);


}
#endif

