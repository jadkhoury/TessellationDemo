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

uniform float u_itpl_alpha;

uniform int u_color_mode;
uniform int u_render_MVP;

vec4 toScreenSpace(vec3 v)
{
    if(u_render_MVP > 0)
        return MVP * vec4(v.x, v.y, v.z, 1);
    else
        return vec4(v.xyz * 0.2, 1) ;
}

vec4 levelColor(uint lvl)
{
    vec4 c = vec4(0.5, 0.5, 0.5, 1);
    uint mod = lvl % 4;
    if(mod == 0) {
        c.r += 0.5;
    } else if (mod == 1) {
        c.g += 0.5;
    } else if (mod == 2) {
        c.b += 0.5;
    }
    return c;
}
Vertex interpolate(Triangle mesh_t, vec2 v, float itpl_alpha)
{
#if FLAG_ITPL_LINEAR
    return lt_interpolateVertex(mesh_t, v);
#elif FLAG_ITPL_PN
    return PNInterpolation(mesh_t, v, itpl_alpha);
#elif FLAG_ITPL_PHONG
    return PhongInterpolation(mesh_t, v, itpl_alpha);
#else
    return lt_interpolateVertex(mesh_t, v);
#endif
}

#endif
