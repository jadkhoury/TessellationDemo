// ------------------------------ VERTEX_SHADER ------------------------------ //
#ifdef VERTEX_SHADER

layout (location = 0) in vec4 p;
layout (location = 1) in vec4 n;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec3 oNormal;
layout (location = 1) out vec2 oTexCoord;

uniform int render_MVP;

void main()
{
    gl_Position = p;
    oNormal = n.xyz;
    oTexCoord = uv;
}
#endif

// --------------------------- TESS_CONTROL_SHADER --------------------------- //
#ifdef TESS_CONTROL_SHADER

// tessellation levels
uniform int uniform_subdiv, uniform_level;
#ifndef LOD_GLSL
uniform mat4 MVP;
#endif

layout(vertices=3) out;

layout(location = 0) in vec3 iNormal[];
layout(location = 1) in vec2 iTexCoord[];

layout(location = 0) out vec3 oNormal[3];
layout(location = 3) out vec2 oTexCoord[3];


void main()
{
    // get data
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    oNormal[gl_InvocationID]            = iNormal[gl_InvocationID];
    oTexCoord[gl_InvocationID]          = iTexCoord[gl_InvocationID];
    gl_TessLevelOuter[gl_InvocationID] = float(1 << uniform_level)*3;
    gl_TessLevelInner[0] = float(1 << uniform_level) * 3;


#ifdef LOD_GLSL
    if(uniform_subdiv <= 0) {
        vec4 p[3];
        vec4 p_mv[3];
        for (int i = 0; i < 3; ++i) {
            p[i] = gl_in[i].gl_Position;
            p_mv[i] = MV * p[i];
        }
        float sum = 0;
        for (int i = 0; i < 3; ++i) {
            gl_TessLevelOuter[i] = lt_computeTessLevel_64(p[i].xyz, p[(i+1)%3].xyz, p_mv[i].xyz, p_mv[(i+1)%3].xyz);
            sum += gl_TessLevelOuter[i];
        }
        gl_TessLevelInner[0] = sum / 3.0;
        gl_TessLevelInner[1] = sum / 3.0;
    }
#endif



}
#endif


// ------------------------- TESS_EVALUATION_SHADER -------------------------- //
#ifdef TESS_EVALUATION_SHADER


uniform float alpha;
uniform int render_MVP;
#ifndef LOD_GLSL
uniform mat4 MVP;
#endif


layout(triangles, equal_spacing, ccw) in;

layout(location = 0) in vec3 iNormal[];
layout(location = 3) in vec2 iTexCoord[];

layout(location = 0) out vec3 oNormal;
layout(location = 1) out vec2 oTexCoord;

void main()
{

    // compute texcoords
    oTexCoord = gl_TessCoord[0]*iTexCoord[0]
            + gl_TessCoord[1]*iTexCoord[1]
            + gl_TessCoord[2]*iTexCoord[2];

    oNormal   = gl_TessCoord[0]*iNormal[0]
            + gl_TessCoord[1]*iNormal[1]
            + gl_TessCoord[2]*iNormal[2];

    // interpolated position
    vec3 p = gl_TessCoord[0]* gl_in[0].gl_Position.xyz
            + gl_TessCoord[1]* gl_in[1].gl_Position.xyz
            + gl_TessCoord[2]* gl_in[2].gl_Position.xyz;

    // final position and normal

    gl_Position = (render_MVP > 0) ? (MVP * vec4(p, 1.0)) : vec4(p, 1.0);
}
#endif
// ------------------------------ GEOMERY_SHADER ----------------------------- //
#ifdef GEOMETRY_SHADER

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (binding = 0) uniform atomic_uint primCount;

layout (location = 0) out vec2 g_leaf_uv;

void main ()
{
    for (int i = 0; i < gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        gl_Position.xyz = gl_Position.xyz;
        g_leaf_uv = vec2(i>>1, i & 0x1);
        EmitVertex();
    }
    atomicCounterIncrement(primCount);
    EndPrimitive();
}
#endif
// ----------------------------- FRAGMENT_SHADER ----------------------------- //
#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 color;

layout (location = 0) in vec2 g_leaf_uv;


// https://github.com/rreusser/glsl-solid-wireframe
float gridFactor (vec2 vBC, float width) {
    vec3 bary = vec3(vBC.x, vBC.y, 1.0 - vBC.x - vBC.y);
    vec3 d = fwidth(bary);
    vec3 a3 = smoothstep(d * (width - 0.5), d * (width + 0.5), bary);
    return min(min(a3.x, a3.y), a3.z);
}

void main()
{
    vec4 white_leaves = vec4(vec3(gridFactor(g_leaf_uv, 1.0)), 1);

    color = white_leaves;
}
#endif
