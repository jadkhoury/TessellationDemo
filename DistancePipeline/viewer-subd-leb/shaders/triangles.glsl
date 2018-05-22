
layout (std430, binding = 0)
buffer TreeBuffer {
	uint keys[2048];
} u_TreeBuffer;

mat3 key_to_xform(in uint key) {
	mat3 xf = mat3(1);

	while (key > 1) {
		float b = key & 0x1;
		float s = 2 * b - 1;

		xf = transpose(
		        mat3(+s*0.5, -0.5  , 0.5,
		             -0.5  , -s*0.5, 0.5,
		             0     , 0     , 1)
		) * xf;
		key = key >> 1u;
	}

	return xf;
}

#ifdef VERTEX_SHADER
void main()
{
	uint key = u_TreeBuffer.keys[gl_InstanceID];
	vec3 std = vec3(gl_VertexID & 1, gl_VertexID >> 1 & 1, 1);
	mat3 xform = key_to_xform(key);
	vec3 pos = xform * std;
	gl_Position = vec4(2.0 * pos.xy - 1.0, 0.0, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) out vec4 o_FragColor;

void main()
{
	o_FragColor = vec4(0);
}
#endif
