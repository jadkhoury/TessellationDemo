#line 2002

#ifndef LOD_GLSL
#define LOD_GLSL

uniform int prim_type;
uniform vec3 cam_pos;
uniform int adaptive_factor;

uniform mat4 M, V, P, MV, MVP, invMV;



// ----------------------- Edge based LoD functions ------------------qqrg----- //
float lt_computePostProjectionSphereExtent(vec3 center, float diameter)
{
    vec4 p = P * vec4(center, 1);
    return abs(diameter * P[1][1] / p.w);
}

float lt_computePostProjectionSphereExtent_mine(vec3 center_mv, float diameter_mv)
{
    vec3 onSphere0_mv = center_mv + vec3(-diameter_mv*0.5, 0, 0);
    vec3 onSphere1_mv = center_mv + vec3( diameter_mv*0.5, 0, 0);
    vec4 onSphere0_mvp = P * vec4(onSphere0_mv, 1.0);
    vec4 onSphere1_mvp = P * vec4(onSphere1_mv, 1.0);
    onSphere0_mvp /= onSphere0_mvp.w;
    onSphere1_mvp /= onSphere1_mvp.w;
    return distance(onSphere0_mvp, onSphere1_mvp);

}

float lt_computeTessLevel_64(vec3 w_p0, vec3 w_p1, vec3 mv_p0, vec3 mv_p1)
{
    vec3 center_mv = (mv_p0 + mv_p1) / 2.0;
    float diameter_mv = distance(mv_p0, mv_p1);

    float projLength = lt_computePostProjectionSphereExtent(center_mv, diameter_mv);

    float LoD = log2(projLength * (1 << adaptive_factor));

//    float t = 0.05;
//    if(mv_p0.z > t && mv_p1.z > t)
//        LoD = 0;

#ifdef DCF_INCLUDE_DC_FRUSTUM_GLSL
//    float d0 = dfc_closestPaneDistance(MVP, w_p0);
//    float d1 = dfc_closestPaneDistance(MVP, w_p1);
//    if(d0 < -t && d1 < -t) {
//        float mean_d = (abs(d0) + abs(d1)) ;
//        LoD = 0;
//    }
#endif
    return clamp(LoD, 0, 31);
}

float lt_computeTessLevel_64(vec4 w_p0, vec4 w_p1, vec4 mv_p0, vec4 mv_p1)
{
    return lt_computeTessLevel_64(w_p0.xyz, w_p1.xyz, mv_p0.xyz, mv_p1.xyz);
}

//#define SPHERE

float edgeToLod(vec4 p0_mvp, vec4 p1_mvp, float edgePixelLengthTarget, float screenResolution, in float cpuLod)
{
#if 1
    vec4 p0_mv = inverse(P) * p0_mvp;
    vec4 p1_mv = inverse(P) * p1_mvp;
    vec4 center_mv = 0.5 * (p0_mv + p1_mv);
    float diameter_mv = distance(p0_mv, p1_mv);
    float edgeLength = lt_computePostProjectionSphereExtent_mine(center_mv.xyz, diameter_mv);
#else
    vec3 p0_ndc = p0_mvp.xyz / p0_mvp.w;
    vec3 p1_ndc = p1_mvp.xyz / p1_mvp.w;
    vec3 edgeVec = p1_ndc - p0_ndc;
    float edgeLength = length(edgeVec);
#endif
    float edgePixelLength = (screenResolution / 2.0) * (edgeLength / (cpuLod + 1.0));
    float tessFactor = edgePixelLength / edgePixelLengthTarget;

    return log2(tessFactor);
}




#endif
