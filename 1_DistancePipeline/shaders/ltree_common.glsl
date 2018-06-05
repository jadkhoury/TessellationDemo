#line 2002

#ifndef LTREE_COMMON_GLSL
#define LTREE_COMMON_GLSL

uniform mat3 shear_mat;
uniform float adaptive_factor;
uniform int prim_type;
uniform vec3 cam_pos;

mat3 toLocal, toWorld;

struct Vertex {
    vec4 p;
    vec4 n;
    vec2 uv;
    vec2 align;
};

layout (std430, binding = 4) readonly buffer Mesh_V
{
    Vertex mesh_v[];
};
layout (std430, binding = 5) readonly buffer Mesh_Q_Idx

{
    uint mesh_q_idx[];
};
layout (std430, binding = 6) readonly buffer Mesh_T_Idx
{
    uint  mesh_t_idx[];
};

float distanceToLod(float x)
{
    float tmp = (x * tan(45.0/2.0))/ (sqrt(2.0) * 2 * adaptive_factor);
    tmp = clamp(tmp, 0.0, 1.0) ;
    return -log2(tmp);
}


void getMeshQuad(in uint idx, out Quad quad)
{
    quad.p[0] = mesh_v[mesh_q_idx[idx+0]].p;
    quad.p[1] = mesh_v[mesh_q_idx[idx+1]].p;
    quad.p[2] = mesh_v[mesh_q_idx[idx+2]].p;
    quad.p[3] = mesh_v[mesh_q_idx[idx+3]].p;

    quad.t[0] = mesh_v[mesh_q_idx[idx+0]].uv;
    quad.t[1] = mesh_v[mesh_q_idx[idx+1]].uv;
    quad.t[2] = mesh_v[mesh_q_idx[idx+2]].uv;
    quad.t[3] = mesh_v[mesh_q_idx[idx+3]].uv;
}

void getMeshTriangle(in uint idx, out Triangle triangle)
{
    triangle.p[0] = mesh_v[mesh_t_idx[idx+0]].p;
    triangle.p[1] = mesh_v[mesh_t_idx[idx+1]].p;
    triangle.p[2] = mesh_v[mesh_t_idx[idx+2]].p;

    triangle.t[0] = mesh_v[mesh_t_idx[idx+0]].uv;
    triangle.t[1] = mesh_v[mesh_t_idx[idx+1]].uv;
    triangle.t[2] = mesh_v[mesh_t_idx[idx+2]].uv;
}



#endif
