#define SQRT_2 1.414213562
#define PI 3.14159265

uniform mat4 u_mvp;
uniform vec3 u_eye_pos;

uniform float u_terrain_size;
uniform vec3  u_sun_dir;

int next_power_of_two(int v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

vec3 irradiance(vec3 n) {
#define C1 0.429043
#define C2 0.511664
#define C3 0.743125
#define C4 0.886227
#define C5 0.247708

#if 0 // Grace Cathedral
#define L00  vec3(+0.79,+0.44,+0.54)
#define L1_1 vec3(+0.39,+0.35,+0.60)
#define L10  vec3(-0.34,-0.18,-0.27)
#define L11  vec3(-0.29,-0.06,+0.01)
#define L2_2 vec3(-0.11,-0.05,-0.12)
#define L2_1 vec3(-0.26,-0.22,-0.47)
#define L20  vec3(-0.16,-0.09,-0.15)
#define L21  vec3(+0.56,+0.21,+0.14)
#define L22  vec3(+0.21,-0.05,-0.30)
#else // Eucalyptus Grove
#define L00  vec3(+0.38,+0.43,+0.45)
#define L1_1 vec3(+0.29,+0.36,+0.41)
#define L10  vec3(+0.04,+0.03,+0.01)
#define L11  vec3(-0.10,-0.10,-0.09)
#define L2_2 vec3(-0.06,-0.06,-0.04)
#define L2_1 vec3(+0.01,-0.01,-0.05)
#define L20  vec3(-0.09,-0.13,-0.15)
#define L21  vec3(-0.06,-0.05,-0.04)
#define L22  vec3(+0.02,-0.00,-0.05)
#endif
	return  C1*L22*(n.x*n.x-n.y*n.y)
	       + C3*L20*n.z*n.z
	       + C4*L00
	       - C5*L20
	       + 2.0*C1*(L2_2*n.x*n.y+L21*n.x*n.z+L2_1*n.y*n.z)
	       + 2.0*C2*(L11*n.x+L1_1*n.y+L10*n.z);
}

#ifdef VERTEX_SHADER
layout(location=0) in vec4 iTransformation; // translation + scale
void main() {
	vec2 st = vec2(gl_VertexID >> 1 & 1, gl_VertexID>0 && gl_VertexID < 3)-0.5;
	vec2 p = st*iTransformation.w + iTransformation.xz;
	gl_Position.xzyw = vec4(p,0,iTransformation.w);
}
#endif


#ifdef TESS_CONTROL_SHADER
layout(vertices=4) out;
void main() {
	// find center and radius of the patch
	vec3 center  = mix(gl_in[gl_InvocationID].gl_Position.xyz,
	                   gl_in[(gl_InvocationID+1)%4].gl_Position.xyz,
	                   0.5);

	center.xz = plane_to_disk(center.xz*2.0/u_terrain_size)
	          * 0.5*u_terrain_size;

//	center.y = clamp(u_eye_pos.y,0.0,6553.5);
	float d = distance(u_eye_pos,center);
	float s = gl_in[0].gl_Position.w;
	float tessLevel2 = clamp(s*64.0*SQRT_2/d, 1.0, 8.0);
	float tessLevel = next_power_of_two(int(tessLevel2));

	// set tess levels
	gl_TessLevelOuter[gl_InvocationID] = tessLevel2;
	gl_TessLevelInner[gl_InvocationID%2] = tessLevel2;//min(length(an-bn)/100.0,16.0);

	// send data
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}

#endif

#ifdef TESS_EVALUATION_SHADER
layout(quads, equal_spacing, ccw) in;
layout(location=0) out vec3 oPosition;
void main() {
	vec3 a = mix(gl_in[0].gl_Position.xyz,
	             gl_in[3].gl_Position.xyz,
	             gl_TessCoord.x);
	vec3 b = mix(gl_in[1].gl_Position.xyz,
	             gl_in[2].gl_Position.xyz,
	             gl_TessCoord.x);
	vec3 p = mix(b, a, gl_TessCoord.y);
//	p/= u_terrain_size*0.5; // [-1,1]
//	oPosition.x = p.x*sqrt(1.0-0.5*p.z*p.z);
//	oPosition.y = 0;
//	oPosition.z = p.z*sqrt(1.0-0.5*p.x*p.x);
//	oPosition*= u_terrain_size*0.5;
	p.xz = plane_to_disk(p.xz*2.0/u_terrain_size)*u_terrain_size*0.5;
	oPosition = p;

#if 0
	vec2 c = oPosition.zx/u_terrain_size*2.0; // -1,1
	oPosition.xz = c.yx * sqrt(1.0-0.5*c*c) * u_terrain_size * 2.0;
//	oPosition.xz = oPosition.xz * sqrt(1.0-0.5*)
#endif
#if 1
	float filter = 3e6/distance(p, u_eye_pos);
        oPosition.y = (displace((p.xz*2.0/u_terrain_size+1.0)*4.0,filter))*1000.0;
#endif
	gl_Position = u_mvp * vec4(oPosition,1);
}
#endif

#ifdef FRAGMENT_SHADER
layout(location=0) in vec3 iPosition;
layout(location=0) out vec3 oColour;
void main() {
        vec2 s;
	float d;
	vec2 p = (iPosition.xz*2.0/u_terrain_size+1.0)*4.0;
	float z = distance(iPosition, u_eye_pos);

	vec3 dpdx = dFdx(iPosition);
	vec3 dpdy = dFdy(iPosition);
	float dp = sqrt(dot(dpdx,dpdx));
	d = displace(p, 1e3/(0.5*dp), s)*1000.0;

        vec3 l = u_sun_dir;
        vec3 n = normalize(vec3(-s,1));

        oColour = vec3(1)*max(dot(l,n),0.0)*0.5;
        oColour+= irradiance(n);
}
#endif // _FRAGMENT_





