#line 4001

#ifndef PHONGINTER
#define PHONGINTER

struct PhongPatch
{
    vec3 termIJ;
    vec3 termJK;
    vec3 termIK;
};

vec3 PIi(vec3 pi, vec3 ni, vec3 q)
{
    vec3 q_minus_p = q - pi;
    return q - dot(q_minus_p, ni) * ni;
}

void getPhongPatch(Triangle t, out PhongPatch oPhongPatch)
{
    vec3 Pi = t.vertex[0].p.xyz;
    vec3 Pj = t.vertex[1].p.xyz;
    vec3 Pk = t.vertex[2].p.xyz;

    vec3 Ni = t.vertex[0].n.xyz;
    vec3 Nj = t.vertex[1].n.xyz;
    vec3 Nk = t.vertex[2].n.xyz;

    oPhongPatch.termIJ = PIi(Pi, Ni, Pj) + PIi(Pj, Nj, Pi);
    oPhongPatch.termJK = PIi(Pj, Nj, Pk) + PIi(Pk, Nk, Pj);
    oPhongPatch.termIK = PIi(Pk, Nk, Pi) + PIi(Pi, Ni, Pk);
}

Vertex Interpolate_phong(Triangle target_T, vec3 uvw, float alpha)
{
    Vertex vertex;
    vec3 Pi = target_T.vertex[0].p.xyz;
    vec3 Pj = target_T.vertex[1].p.xyz;
    vec3 Pk = target_T.vertex[2].p.xyz;


    PhongPatch phongPatch;
    getPhongPatch(target_T, phongPatch);
    vec3 tc1 = uvw;
    vec3 tc2 = tc1*tc1;

    // UVs
    vertex.uv  = uvw[2]*target_T.vertex[0].uv
            + uvw[0]*target_T.vertex[1].uv
            + uvw[1]*target_T.vertex[2].uv;

    // Normal
    vertex.n = tc1[0] * target_T.vertex[0].n
            + tc1[1] * target_T.vertex[1].n
            + tc1[2] * target_T.vertex[2].n;
    vertex.n = normalize(vertex.n);

    // Position
    vec3 barPos = tc1[0]*Pi + tc1[1]*Pj + tc1[2]*Pk;

    vec3 phongPos   = tc2[0] * Pi
            + tc2[1] * Pj
            + tc2[2] * Pk
            + tc1[0] * tc1[1] * phongPatch.termIJ
            + tc1[1] * tc1[2] * phongPatch.termJK
            + tc1[2] * tc1[0] * phongPatch.termIK;

    alpha *= 0.5;
    phongPos = (1.0-alpha)*barPos + alpha*phongPos;

    vertex.p = vec4(phongPos, 1);

    return vertex;

}

Vertex PhongInterpolation(Triangle mesh_t, vec2 uv, float alpha)
{
    float u = uv.x, v = uv.y, w = 1-u-v;
    vec3 uvw = vec3(w,v,u);
    uvw = uvw / (u+v+w);

    return Interpolate_phong(mesh_t, uvw, alpha);
}




#endif

