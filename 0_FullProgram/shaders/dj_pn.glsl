
// --------------------------- TESS_CONTROL_SHADER --------------------------- //
#ifdef TESS_CONTROL_SHADER

// PN patch data
struct PnPatch
{
 float b210;
 float b120;
 float b021;
 float b012;
 float b102;
 float b201;
 float b111;
 float n110;
 float n011;
 float n101;
};

// tessellation levels
uniform int uniform_level;

layout(vertices=3) out;

layout(location = 0) in vec3 iNormal[];
layout(location = 1) in vec2 iTexCoord[];

layout(location = 0) out vec3 oNormal[3];
layout(location = 3) out vec2 oTexCoord[3];
layout(location = 6) out PnPatch oPnPatch[3];

float wij(int i, int j)
{
 return dot(gl_in[j].gl_Position.xyz - gl_in[i].gl_Position.xyz, iNormal[i]);
}

float vij(int i, int j)
{
 vec3 Pj_minus_Pi = gl_in[j].gl_Position.xyz
				  - gl_in[i].gl_Position.xyz;
 vec3 Ni_plus_Nj  = iNormal[i]+iNormal[j];
 return 2.0*dot(Pj_minus_Pi, Ni_plus_Nj)/dot(Pj_minus_Pi, Pj_minus_Pi);
}

void main()
{
 // get data
 gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
 oNormal[gl_InvocationID]            = iNormal[gl_InvocationID];
 oTexCoord[gl_InvocationID]          = iTexCoord[gl_InvocationID];

 // set base
 float P0 = gl_in[0].gl_Position[gl_InvocationID];
 float P1 = gl_in[1].gl_Position[gl_InvocationID];
 float P2 = gl_in[2].gl_Position[gl_InvocationID];
 float N0 = iNormal[0][gl_InvocationID];
 float N1 = iNormal[1][gl_InvocationID];
 float N2 = iNormal[2][gl_InvocationID];

 // compute control points
 oPnPatch[gl_InvocationID].b210 = (2.0*P0 + P1 - wij(0,1)*N0)/3.0;
 oPnPatch[gl_InvocationID].b120 = (2.0*P1 + P0 - wij(1,0)*N1)/3.0;
 oPnPatch[gl_InvocationID].b021 = (2.0*P1 + P2 - wij(1,2)*N1)/3.0;
 oPnPatch[gl_InvocationID].b012 = (2.0*P2 + P1 - wij(2,1)*N2)/3.0;
 oPnPatch[gl_InvocationID].b102 = (2.0*P2 + P0 - wij(2,0)*N2)/3.0;
 oPnPatch[gl_InvocationID].b201 = (2.0*P0 + P2 - wij(0,2)*N0)/3.0;
 float E = ( oPnPatch[gl_InvocationID].b210
		   + oPnPatch[gl_InvocationID].b120
		   + oPnPatch[gl_InvocationID].b021
		   + oPnPatch[gl_InvocationID].b012
		   + oPnPatch[gl_InvocationID].b102
		   + oPnPatch[gl_InvocationID].b201 ) / 6.0;
 float V = (P0 + P1 + P2)/3.0;
 oPnPatch[gl_InvocationID].b111 = E + (E - V)*0.5;
 oPnPatch[gl_InvocationID].n110 = N0+N1-vij(0,1)*(P1-P0);
 oPnPatch[gl_InvocationID].n011 = N1+N2-vij(1,2)*(P2-P1);
 oPnPatch[gl_InvocationID].n101 = N2+N0-vij(2,0)*(P0-P2);

 // set tess levels
 gl_TessLevelOuter[gl_InvocationID] = float(uniform_level);
 gl_TessLevelInner[0] = float(uniform_level);
}
#endif


// ------------------------- TESS_EVALUATION_SHADER -------------------------- //
#ifdef TESS_EVALUATION_SHADER

// PN patch data
struct PnPatch
{
 float b210;
 float b120;
 float b021;
 float b012;
 float b102;
 float b201;
 float b111;
 float n110;
 float n011;
 float n101;
};

uniform mat4 MVP; // mvp
uniform float alpha;          // controls the deformation

layout(triangles, fractional_odd_spacing, ccw) in;

layout(location = 0) in vec3 iNormal[];
layout(location = 3) in vec2 iTexCoord[];
layout(location = 6) in PnPatch iPnPatch[];

layout(location = 0) out vec3 oNormal;
layout(location = 1) out vec2 oTexCoord;

#define b300    gl_in[0].gl_Position.xyz
#define b030    gl_in[1].gl_Position.xyz
#define b003    gl_in[2].gl_Position.xyz
#define n200    iNormal[0]
#define n020    iNormal[1]
#define n002    iNormal[2]
#define uvw     gl_TessCoord

void main()
{
 vec3 uvwSquared = uvw*uvw;
 vec3 uvwCubed   = uvwSquared*uvw;

 // extract control points
 vec3 b210 = vec3(iPnPatch[0].b210, iPnPatch[1].b210, iPnPatch[2].b210);
 vec3 b120 = vec3(iPnPatch[0].b120, iPnPatch[1].b120, iPnPatch[2].b120);
 vec3 b021 = vec3(iPnPatch[0].b021, iPnPatch[1].b021, iPnPatch[2].b021);
 vec3 b012 = vec3(iPnPatch[0].b012, iPnPatch[1].b012, iPnPatch[2].b012);
 vec3 b102 = vec3(iPnPatch[0].b102, iPnPatch[1].b102, iPnPatch[2].b102);
 vec3 b201 = vec3(iPnPatch[0].b201, iPnPatch[1].b201, iPnPatch[2].b201);
 vec3 b111 = vec3(iPnPatch[0].b111, iPnPatch[1].b111, iPnPatch[2].b111);

 // extract control normals
 vec3 n110 = normalize(vec3(iPnPatch[0].n110,
							iPnPatch[1].n110,
							iPnPatch[2].n110));
 vec3 n011 = normalize(vec3(iPnPatch[0].n011,
							iPnPatch[1].n011,
							iPnPatch[2].n011));
 vec3 n101 = normalize(vec3(iPnPatch[0].n101,
							iPnPatch[1].n101,
							iPnPatch[2].n101));

 // compute texcoords
 oTexCoord  = gl_TessCoord[2]*iTexCoord[0]
			+ gl_TessCoord[0]*iTexCoord[1]
			+ gl_TessCoord[1]*iTexCoord[2];

 // normal
 vec3 barNormal = gl_TessCoord[2]*iNormal[0]
				+ gl_TessCoord[0]*iNormal[1]
				+ gl_TessCoord[1]*iNormal[2];
 vec3 pnNormal  = n200*uvwSquared[2]
				+ n020*uvwSquared[0]
				+ n002*uvwSquared[1]
				+ n110*uvw[2]*uvw[0]
				+ n011*uvw[0]*uvw[1]
				+ n101*uvw[2]*uvw[1];
 oNormal = alpha*pnNormal + (1.0-alpha)*barNormal;

 // compute interpolated pos
 vec3 barPos = gl_TessCoord[2]*b300
			 + gl_TessCoord[0]*b030
			 + gl_TessCoord[1]*b003;

 // save some computations
 uvwSquared *= 3.0;

 // compute PN position
 vec3 pnPos  = b300*uvwCubed[2]
			 + b030*uvwCubed[0]
			 + b003*uvwCubed[1]
			 + b210*uvwSquared[2]*uvw[0]
			 + b120*uvwSquared[0]*uvw[2]
			 + b201*uvwSquared[2]*uvw[1]
			 + b021*uvwSquared[0]*uvw[1]
			 + b102*uvwSquared[1]*uvw[2]
			 + b012*uvwSquared[1]*uvw[0]
			 + b111*6.0*uvw[0]*uvw[1]*uvw[2];

 // final position and normal
 vec3 finalPos = (1.0-alpha)*barPos + alpha*pnPos;
 gl_Position   = MVP * vec4(finalPos,1.0);
}
#endif
