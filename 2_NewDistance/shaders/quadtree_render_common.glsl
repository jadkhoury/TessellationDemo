#line 5001
#ifndef RENDER_COMMON_GLSL
#define RENDER_COMMON_GLSL

const vec4 RED     = vec4(1,0,0,1);
const vec4 GREEN   = vec4(0,1,0,1);
const vec4 BLUE    = vec4(0,0,1,1);
const vec4 CYAN    = vec4(0,1,1,1);
const vec4 MAGENTA = vec4(1,0,0.5,1);
const vec4 YELLOW  = vec4(1,1,0,1);
const vec4 BLACK   = vec4(0,0,0,1);

uniform int u_displace_on;

uniform int u_morph_debug;
uniform float u_morph_k;

uniform float u_itpl_alpha;

uniform int u_color_mode;
uniform int u_render_MVP;

layout(std140, binding = CAM_HEIGHT_B) uniform Cam_Height
{
    float cam_height_ssbo;
};

// based on Filip Strugar's CDLOD paper (until intPart & signVec)
vec2 morphVertex(vec2 leaf_p,  float key_lod, float target_lod, out uint morphed)
{
    float tessLevel = clamp(key_lod -  target_lod, 0.0, 1.0);
    float morphK = smoothstep(0.4, 0.5, tessLevel);
    // out variable for shading
    morphed = (morphK > 0 && morphK < 1) ? 1 : 0;

    // nb of intervals per side of node primitive
    float patchTessFactor = 1u << uint(u_cpu_lod);
    vec2 fracPart = fract(leaf_p * patchTessFactor * 0.5) * 2.0 / patchTessFactor;
    vec2 intPart = floor(leaf_p * patchTessFactor * 0.5);
    vec2 signVec = mod(intPart, 2.0) * vec2(-2.0) + vec2(1.0);

    return (leaf_p - signVec * fracPart * morphK);
}

// based on Filip Strugar's CDLOD paper (until intPart & signVec)
vec2 morphVertexDebug(vec2 leaf_p, float k)
{
    // nb of intervals per side of node primitive
    float patchTessFactor = 1u << uint(u_cpu_lod);
    float tmp = patchTessFactor * 0.5;
    vec2 fracPart = fract(leaf_p * tmp) / tmp;
    vec2 intPart = floor(leaf_p * tmp);
    vec2 signVec = mod(intPart, 2.0) * vec2(-2.0) + vec2(1.0);

    return (leaf_p - signVec * fracPart * k);
}

vec4 toScreenSpace(vec3 v)
{
    if(u_render_MVP > 0)
        return MVP * vec4(v.x, v.y, v.z, 1);
    else
        return vec4(v.xyz * 0.2, 1) ;
}

vec4 levelColor(uint lvl, uint morphed)
{
    vec4 c = vec4(0.0, 0.0, 0.9, 1);
    c.r += (float(lvl) / 10.0);
    if (lvl % 2 == 1) {
        c.g += 0.5;
    }
    c = mix(c, RED, float(morphed)*0.5);
    return c;
}

Vertex interpolate(Triangle mesh_t, vec2 v, float itpl_alpha)
{
#ifdef ITPL_LINEAR
    return lt_interpolateVertex(mesh_t, v);
#elif defined(ITPL_PN)
    return PNInterpolation(mesh_t, v, itpl_alpha);
#elif defined(ITPL_PHONG)
    return PhongInterpolation(mesh_t, v, itpl_alpha);
#else
    return lt_interpolateVertex(mesh_t, v);
#endif
}

#endif
