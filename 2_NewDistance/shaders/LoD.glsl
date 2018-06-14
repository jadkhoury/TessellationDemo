#line 2002

#ifndef LOD_GLSL
#define LOD_GLSL

uniform int prim_type;
uniform vec3 cam_pos;
uniform float adaptive_factor;

uniform mat4 M, V, P, MV, MVP, invMV;
uniform float fov;

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



#endif // LOD_GLSL
