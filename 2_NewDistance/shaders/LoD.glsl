#line 2002

#ifndef LOD_GLSL
#define LOD_GLSL

uniform int prim_type;
uniform float adaptive_factor;



//uniform mat4 M, V, P, MV, MVP, invMV;
//uniform vec3 cam_pos;
//uniform float fov;

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

const vec2 triangle_centroid = vec2(1.0/3.0, 1.0/3.0);

const float SQRT_2 = sqrt(2);
const float TAN_FOV = tan(radians(fov/2.0));

float distanceToLod(vec3 pos)
{
    float x = distance(pos, cam_pos);
    float tmp = (x * TAN_FOV)/ (SQRT_2 * 2 * adaptive_factor);
    tmp = clamp(tmp, 0.0, 1.0) ;
    return -log2(tmp);
}

float computeTessLevelFromKey(uvec4 key, bool parent) {
    vec4 mesh_p = lt_Leaf_to_MeshPrimitive(triangle_centroid, key, parent, prim_type);
    mesh_p = M * mesh_p;

    return distanceToLod(mesh_p.xyz);
}

void computeTessLvlWithParent(uvec4 key, out float lvl, out float parent_lvl) {
    vec4 p_mesh, pp_mesh;
    lt_Leaf_n_Parent_to_MeshPrimitive(triangle_centroid, key, p_mesh, pp_mesh, prim_type);
    p_mesh  = M * p_mesh;
    pp_mesh = M * pp_mesh;

    lvl        = distanceToLod(p_mesh.xyz);
    parent_lvl = distanceToLod(pp_mesh.xyz);
}

vec3 nvertex(vec3 bmin, vec3 bmax, vec3 n)
{
    bvec3 b = greaterThan(n, vec3(0));
    return mix(bmin, bmax, b);
}

bool culltest(mat4 mvp, vec3 bmin, vec3 bmax)
{
    float a = 1.0;

    for (int i = 0; i < 6 && a >= 0.0; ++i) {
        vec3 n = nvertex(bmin, bmax, frustum_planes[i].xyz);

        a = dot(vec4(n, 1.0), frustum_planes[i]);
    }

    return (a >= 0.0);
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
