#line 3002

float displace(vec3 p, float screen_resolution) {
	p+=2.0;
	const float H = 0.99;
	const float lacunarity = 2.0;
	const float max_octaves = 8.0;
	float frequency = 1.5;
	float octaves = clamp(log2(screen_resolution) - 2.0,0.0,max_octaves);
	float value = Cellular3D(p)*4.5;

	for(float i=0.0; i<octaves-1.0; i+=1.0) {
		value+= SimplexPerlin3D(p) * pow(frequency,-H);
		p*= lacunarity;
		frequency*= lacunarity;
	}
	value+= fract(octaves)*SimplexPerlin3D(p)*pow(frequency,-H);
	value*=0.1;
	return value;
}

float displace(vec3 p, float screen_resolution, out vec3 gradient) {
	p+=2.0;
	const float H = 0.99; // TODO solve this issue with normals ...
	const float lacunarity = 2.0;
	const float max_octaves = 10.0;
	float frequency = 1.5;
	float octaves = clamp(log2(screen_resolution) - 2.0,0.0,max_octaves);
	vec4 value = Cellular3D_Deriv(p)*4.5;

	for(float i=0.0; i<octaves-1.0; i+=1.0) {
		value+= SimplexPerlin3D_Deriv(p) * pow(frequency,-H)
				* vec4(1,vec3(pow(lacunarity,i)));
		p*= lacunarity;
		frequency*= lacunarity;
	}
	value+= fract(octaves)*SimplexPerlin3D(p)*pow(frequency,-H);
	value*=0.1;
	gradient = value.yzw;
	return value.x;
}


// nice solo: vec4 x6 = SimplexPerlin3D_Deriv(p*4.0)/16.0;

vec3 rock_normal(vec3 p, float screen_resolution, out float d) {
	vec3 f;
	d = displace(p,screen_resolution,f);
	return 2.0*(p - f*(d+1.0));
}

vec3 albedo(vec3 p, vec3 n) {

	//return vec3(Cellular3D(p));
	float k = sin(dot(p+SimplexPerlin3D(p*2.0),vec3(1)))*0.5+0.5;
	//	return vec3(k);//*vec3(0.423,0.561,0.004);
	//return vec3(0.337,0.302,0.223);
	return mix(vec3(0.337,0.302,0.223), vec3(0.237,0.202,0.123)*0.5,
			   clamp(k*n.y,0.0,1.0));
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


