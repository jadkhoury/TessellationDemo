#line 2002

#ifndef LOD_GLSL
#define LOD_GLSL

uniform float u_lod_factor;

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
float TAN_FOV = tan(radians(fov/2.0));

float distanceToLod(vec3 pos)
{
    float d = distance(pos, cam_pos);
    float lod = (d * u_lod_factor);
    lod = clamp(lod, 0.0, 1.0) ;
    return -log2(lod);
}

void computeTessLvlWithParent(uvec4 key, float height, out float lvl, out float parent_lvl) {
    vec4 p_mesh, pp_mesh;
    lt_Leaf_n_Parent_to_MeshPosition(triangle_centroid, key, p_mesh, pp_mesh);
    p_mesh  = M * p_mesh;
    pp_mesh = M * pp_mesh;
    p_mesh.z = height;
    pp_mesh.z = height;

    lvl        = distanceToLod(p_mesh.xyz);
    parent_lvl = distanceToLod(pp_mesh.xyz);
}

void computeTessLvlWithParent(uvec4 key, out float lvl, out float parent_lvl) {
    vec4 p_mesh, pp_mesh;
    lt_Leaf_n_Parent_to_MeshPosition(triangle_centroid, key, p_mesh, pp_mesh);
    p_mesh  = M * p_mesh;
    pp_mesh = M * pp_mesh;

    lvl        = distanceToLod(p_mesh.xyz);
    parent_lvl = distanceToLod(pp_mesh.xyz);
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
