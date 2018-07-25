#line 2002

#ifndef LOD_GLSL
#define LOD_GLSL

uniform int u_poly_type;
uniform float u_adaptive_factor;
uniform int u_mode;
uniform float u_target_edge_length;
uniform int u_screen_res;
uniform int u_cpu_lod;
uniform int u_morph_on;


#define BUFFER_HEIGHT

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
    float f;

    if (u_mode == TERRAIN) {
        float leaf_subdiv = float(1 << uint(u_cpu_lod+1-u_morph_on));
        f = u_screen_res/(SQRT_2 * u_target_edge_length * leaf_subdiv);
    } else {
        f = u_adaptive_factor;
    }
    float lod = (d * TAN_FOV)/ (SQRT_2 * f);
    lod = clamp(lod, 0.0, 1.0) ;
    return -log2(lod);
}

void computeTessLvlWithParent(uvec4 key, float height, out float lvl, out float parent_lvl) {
    vec4 p_mesh, pp_mesh;
    lt_Leaf_n_Parent_to_MeshPosition(triangle_centroid, key, p_mesh, pp_mesh, u_poly_type);
    p_mesh  = M * p_mesh;
    pp_mesh = M * pp_mesh;
    p_mesh.z = height;
    pp_mesh.z = height;


    lvl        = distanceToLod(p_mesh.xyz);
    parent_lvl = distanceToLod(pp_mesh.xyz);
}

void computeTessLvlWithParent(uvec4 key, out float lvl, out float parent_lvl) {
    vec4 p_mesh, pp_mesh;
    lt_Leaf_n_Parent_to_MeshPosition(triangle_centroid, key, p_mesh, pp_mesh, u_poly_type);
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
