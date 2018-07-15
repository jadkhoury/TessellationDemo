#ifndef NOISE_GLSL
#define NOISE_GLSL

const float H = 0.96;
const float lacunarity = 1.99;

float displace(vec2 p, float screen_resolution)
{
	const float max_octaves = 16.0;
	float frequency = 1.5;
	float octaves = clamp(log2(screen_resolution) - 2.0,0.0,max_octaves);
	float value = 0.0;

	for(float i=0.0; i<octaves-1.0; i+=1.0) {
		value+= SimplexPerlin2D(p) * pow(frequency,-H);
		p*= lacunarity;
		frequency*= lacunarity;
	}
	value+= fract(octaves)*SimplexPerlin2D(p)*pow(frequency,-H);
	return value;
}

float displace(vec2 p, float screen_resolution, out vec2 gradient)
{
	const float max_octaves = 24.0;
	float frequency = 1.5;
	float octaves = clamp(log2(screen_resolution) - 2.0,0.0,max_octaves);
	vec3 value = vec3(0);

	for(float i=0.0; i<octaves-1.0; i+=1.0) {
		vec3 v = SimplexPerlin2D_Deriv(p);
//		v.yz*= sign(v.x);
		value+= v * pow(frequency,-H)
		      * vec3(1,vec2(pow(lacunarity,i)));
		p*= lacunarity;
		frequency*= lacunarity;
	}
//	vec3 v = SimplexPerlin2D_Deriv(p);
//	v.yz*= sign(v.x);
	value+= fract(octaves)*SimplexPerlin2D_Deriv(p)
	      * pow(frequency,-H) * vec3(1,vec2(pow(lacunarity,octaves)));
	gradient = value.yz;
	return value.x;
}

const float height_factor = 0.3;

vec3 displaceVertex(vec3 v, vec3 eye) {
    float f = 3e3 / distance(v, eye);
    v.z = displace(v.xy, f) * height_factor;
    return v;
}

vec4 displaceVertex(vec4 v, vec3 eye) {
    return vec4(displaceVertex(v.xyz, eye), v.w);
}

#endif
