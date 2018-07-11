#line 70001
#ifndef PNINTERP_GLSL
#define PNINTERP_GLSL

#ifndef LTREE_GLSL
struct Vertex {
    vec4 p;
    vec4 n;
    vec2 uv;
    vec2 align;
};
#endif


// PN patch data
struct PnPatch
{
    vec3 b300;
    vec3 b030;
    vec3 b003;
    vec3 b210;
    vec3 b120;
    vec3 b021;
    vec3 b012;
    vec3 b102;
    vec3 b201;
    vec3 b111;

    vec3 n200;
    vec3 n020;
    vec3 n002;
    vec3 n110;
    vec3 n011;
    vec3 n101;
};

float wij(vec3 pi, vec3 pj, vec3 ni)
{
    return dot(pj - pi, ni);
}

float vij(vec3 pi, vec3 pj, vec3 ni, vec3 nj)
{
    vec3 Pj_minus_Pi = pj - pi;
    vec3 Ni_plus_Nj  = ni + nj;
    return 2.0*dot(Pj_minus_Pi, Ni_plus_Nj)/dot(Pj_minus_Pi, Pj_minus_Pi);
}

void getPnPatch(Triangle t, out PnPatch oPnPatch)
{
    vec3 P0 = t.vertex[0].p.xyz;
    vec3 P1 = t.vertex[1].p.xyz;
    vec3 P2 = t.vertex[2].p.xyz;

    vec3 N0 = t.vertex[0].n.xyz;
    vec3 N1 = t.vertex[1].n.xyz;
    vec3 N2 = t.vertex[2].n.xyz;

    oPnPatch.b300 = P0;
    oPnPatch.b030 = P1;
    oPnPatch.b003 = P2;
    oPnPatch.n200 = normalize(N0);
    oPnPatch.n020 = normalize(N1);
    oPnPatch.n002 = normalize(N2);

    oPnPatch.b210 = (2.0*P0 + P1 - wij(P0,P1,N0)*N0)/3.0;
    oPnPatch.b120 = (2.0*P1 + P0 - wij(P1,P0,N1)*N1)/3.0;
    oPnPatch.b021 = (2.0*P1 + P2 - wij(P1,P2,N1)*N1)/3.0;
    oPnPatch.b012 = (2.0*P2 + P1 - wij(P2,P1,N2)*N2)/3.0;
    oPnPatch.b102 = (2.0*P2 + P0 - wij(P2,P0,N2)*N2)/3.0;
    oPnPatch.b201 = (2.0*P0 + P2 - wij(P0,P2,N0)*N0)/3.0;

    vec3 E = ( oPnPatch.b210 + oPnPatch.b120 + oPnPatch.b021 +
               oPnPatch.b012 + oPnPatch.b102 + oPnPatch.b201 ) / 6.0;
    vec3 V = (P0 + P1 + P2)/3.0;

    oPnPatch.b111 = E + (E - V) * 0.5;

    oPnPatch.n110 = normalize(N0 + N1 - vij(P0,P1,N0,N1) * (P1-P0));
    oPnPatch.n011 = normalize(N1 + N2 - vij(P1,P2,N1,N2) * (P2-P1));
    oPnPatch.n101 = normalize(N2 + N0 - vij(P2,P0,N2,N0) * (P0-P2));
}


void Interpolate_pn(Triangle target_T, vec3 uvw, float alpha, out vec4 result_p, out vec4 result_n)
{
    PnPatch pnPatch;
    getPnPatch(target_T, pnPatch);

    vec3 uvwSquared = uvw*uvw;
    vec3 uvwCubed   = uvwSquared*uvw;

    // normal
    vec3 barNormal = uvw[2]*pnPatch.n200
            + uvw[0]*pnPatch.n020
            + uvw[1]*pnPatch.n002;

    vec3 pnNormal  = pnPatch.n200*uvwSquared[2]
            + pnPatch.n020*uvwSquared[0]
            + pnPatch.n002*uvwSquared[1]
            + pnPatch.n110*uvw[2]*uvw[0]
            + pnPatch.n011*uvw[0]*uvw[1]
            + pnPatch.n101*uvw[2]*uvw[1];

    result_n = normalize(vec4(alpha*pnNormal + (1.0-alpha)*barNormal, 0));

    // compute interpolated pos
    vec3 barPos = uvw[2]*pnPatch.b300
            + uvw[0]*pnPatch.b030
            + uvw[1]*pnPatch.b003;

    // save some computations
    uvwSquared *= 3.0;

    // compute PN position
    vec3 pnPos  = pnPatch.b300*uvwCubed[2]
            + pnPatch.b030*uvwCubed[0]
            + pnPatch.b003*uvwCubed[1]
            + pnPatch.b210*uvwSquared[2]*uvw[0]
            + pnPatch.b120*uvwSquared[0]*uvw[2]
            + pnPatch.b201*uvwSquared[2]*uvw[1]
            + pnPatch.b021*uvwSquared[0]*uvw[1]
            + pnPatch.b102*uvwSquared[1]*uvw[2]
            + pnPatch.b012*uvwSquared[1]*uvw[0]
            + pnPatch.b111*6.0*uvw[0]*uvw[1]*uvw[2];

    //	// compute texcoords
    //    oTexCoord  = gl_TessCoord[2]*iTexCoord[0]
    //               + gl_TessCoord[0]*iTexCoord[1]
    //               + gl_TessCoord[1]*iTexCoord[2];


    // final position and normal
    vec3 finalPos = (1.0-alpha)*barPos + alpha*pnPos;

    result_p = vec4(finalPos, 1.0);
}

void PNInterpolation(uvec4 key, vec2 uv, int poly_type, float alpha, out vec4 final_pos, out vec4 final_n)
{
    uint meshPolygonID = key.z;
    uint rootID = key.w;
    Triangle mesh_t;
    if (poly_type == TRIANGLES)
        lt_getMeshTriangle(meshPolygonID, mesh_t);
    else
        lt_getQuadMeshTriangle(meshPolygonID, rootID, mesh_t);

    float u = uv.x, v = uv.y, w = 1.0-u-v;
    vec3 uvw = vec3(v, u, w);
    uvw = uvw / (u+v+w);

    Interpolate_pn(mesh_t, uvw, alpha, final_pos, final_n);
}
#endif
