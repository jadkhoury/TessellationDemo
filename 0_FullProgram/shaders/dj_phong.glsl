
// --------------------------- TESS_CONTROL_SHADER --------------------------- //
#ifdef TESS_CONTROL_SHADER

// Phong tess patch data
struct PhongPatch
{
 float termIJ;
 float termJK;
 float termIK;
};

uniform int uniform_level;

layout(vertices=3) out;

layout(location = 0)   in vec3 iNormal[];
layout(location = 1)   in vec2 iTexCoord[];

layout(location=0) out vec3 oNormal[3];
layout(location=3) out vec2 oTexCoord[3];
layout(location=6) out PhongPatch oPhongPatch[3];

#define Pi  gl_in[0].gl_Position.xyz
#define Pj  gl_in[1].gl_Position.xyz
#define Pk  gl_in[2].gl_Position.xyz

float PIi(int i, vec3 q)
{
 vec3 q_minus_p = q - gl_in[i].gl_Position.xyz;
 return q[gl_InvocationID] - dot(q_minus_p, iNormal[i])
						   * iNormal[i][gl_InvocationID];
}

void main()
{
 // get data
 gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
 oNormal[gl_InvocationID]   = iNormal[gl_InvocationID];
 oTexCoord[gl_InvocationID] = iTexCoord[gl_InvocationID];

 // compute patch data
 oPhongPatch[gl_InvocationID].termIJ = PIi(0,Pj) + PIi(1,Pi);
 oPhongPatch[gl_InvocationID].termJK = PIi(1,Pk) + PIi(2,Pj);
 oPhongPatch[gl_InvocationID].termIK = PIi(2,Pi) + PIi(0,Pk);

 // tesselate
 gl_TessLevelOuter[gl_InvocationID] = float(uniform_level);
 gl_TessLevelInner[0] = float(uniform_level);
}
#endif

// ----------------------- TESS_EVALUATION_SHADER -------------------------- //
#ifdef TESS_EVALUATION_SHADER

// Phong tess patch data
struct PhongPatch
{
 float termIJ;
 float termJK;
 float termIK;
};

uniform float alpha;
uniform mat4  MVP;

layout(triangles, fractional_odd_spacing, ccw) in;

layout(location=0) in vec3 iNormal[];
layout(location=3) in vec2 iTexCoord[];
layout(location=6) in PhongPatch iPhongPatch[];

layout(location=0) out vec3 oNormal;
layout(location=1) out vec2 oTexCoord;

#define Pi  gl_in[0].gl_Position.xyz
#define Pj  gl_in[1].gl_Position.xyz
#define Pk  gl_in[2].gl_Position.xyz
#define tc1 gl_TessCoord

void main()
{
 // precompute squared tesscoords
 vec3 tc2 = tc1*tc1;

 // compute texcoord and normal
 oTexCoord = gl_TessCoord[0]*iTexCoord[0]
		   + gl_TessCoord[1]*iTexCoord[1]
		   + gl_TessCoord[2]*iTexCoord[2];
 oNormal   = gl_TessCoord[0]*iNormal[0]
		   + gl_TessCoord[1]*iNormal[1]
		   + gl_TessCoord[2]*iNormal[2];

 // interpolated position
 vec3 barPos = gl_TessCoord[0]*Pi
			 + gl_TessCoord[1]*Pj
			 + gl_TessCoord[2]*Pk;

 // build terms
 vec3 termIJ = vec3(iPhongPatch[0].termIJ,
					iPhongPatch[1].termIJ,
					iPhongPatch[2].termIJ);
 vec3 termJK = vec3(iPhongPatch[0].termJK,
					iPhongPatch[1].termJK,
					iPhongPatch[2].termJK);
 vec3 termIK = vec3(iPhongPatch[0].termIK,
					iPhongPatch[1].termIK,
					iPhongPatch[2].termIK);

 // phong tesselated pos
 vec3 phongPos   = tc2[0]*Pi
				 + tc2[1]*Pj
				 + tc2[2]*Pk
				 + tc1[0]*tc1[1]*termIJ
				 + tc1[1]*tc1[2]*termJK
				 + tc1[2]*tc1[0]*termIK;

 // final position
 vec3 finalPos = (1.0-alpha)*barPos + alpha*phongPos;
 gl_Position   = MVP * vec4(finalPos,1.0);
}
#endif
