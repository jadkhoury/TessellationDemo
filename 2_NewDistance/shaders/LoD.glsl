#line 2002

#ifndef LOD_GLSL
#define LOD_GLSL

uniform float u_lod_factor;
uniform int u_mode;
uniform float u_target_edge_length;
uniform int u_screen_res;
uniform int u_cpu_lod;
uniform int u_morph_on;



//#define BUFFER_HEIGHT

layout(std140, binding = 0) uniform TransformBlock
{
    mat4 M;
    mat4 V;
    mat4 P;
    mat4 MVP;
    mat4 MV;
    mat4 invMV;
    vec4 frustum_planes[6];

    vec3 cam_pos;
    float fov;
};

const float SQRT_2 = sqrt(2);
float TAN_FOV = tan(radians(fov/2.0));

float distanceToLod(float d)
{
    return - 2.0f * log2(clamp(d * u_lod_factor, 0.0f, 1.0f));
}

bool culltest(mat4 mvp, vec3 bmin, vec3 bmax)
{
    bool inside = true;
    for (int i = 0; i < 6; ++i) {
        bvec3 b = greaterThan(frustum_planes[i].xyz, vec3(0));
        vec3 n = mix(bmin, bmax, b);
        inside = inside && (dot(vec4(n, 1.0), frustum_planes[i]) >= 0);
    }
    return inside;
}

float closestPaneDistance(in mat4 MVP, in vec3 p)
{
    float signed_d = 0, min_d = 10e6;
    bool inside = true;
    for (int i = 0; i < 6 ; ++i) {
        signed_d = dot(vec4(p,1), normalize(frustum_planes[i]));
        if(signed_d < min_d){
            min_d = signed_d;
        }
    }
    return min_d;
}

#endif // LOD_GLSL
