#line 2002

#ifndef LOD_GLSL
#define LOD_GLSL

uniform int prim_type;
uniform vec3 cam_pos;
uniform float adaptive_factor;

uniform mat4 M, V, P, MV, MVP, invMV;
uniform float fov;

const vec2 triangle_centroid = vec2(1.0/3.0, 1.0/3.0);

float distanceToLod(vec3 pos)
{
    float x = distance(pos, cam_pos);
    float tmp = (x * tan(radians(fov)))/ (sqrt(2.0) * 2 * adaptive_factor);
    tmp = clamp(tmp, 0.0, 1.0) ;
    return -log2(tmp);
}

float computeTessLevelFromKey(uvec4 key, bool parent) {
    vec4 mesh_p = lt_Leaf_to_MeshPrimitive(triangle_centroid, key, parent, prim_type);
    mesh_p = M * mesh_p;

    return distanceToLod(mesh_p.xyz);
}

#endif // LOD_GLSL
