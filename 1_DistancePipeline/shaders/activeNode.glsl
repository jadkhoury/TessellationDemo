
uniform float u_MaxLevel;
uniform uint u_ActiveNode;

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
	
}
#endif

#ifdef GEOMETRY_SHADER
layout(points) in;
layout(line_strip, max_vertices = 16) out;

void main()
{
	uint key = u_ActiveNode;
	if (key > 0) {
		gl_Position = vec4(0.0, 1.0 / float(sqrt(u_MaxLevel)), 0, 1);
		EmitVertex();

		vec2 p = vec2(0.0, 1.0);
		int cnt = findMSB(key);
		for (int i = 0; i < cnt; ++i) {
			uint mask = 1 << (cnt - 1 - i);
			uint b = key & mask;
			float dx = 1.0 / float(1 << i);

			p.x+= b > 0 ? dx : -dx;
			p.y-= 1 / float(u_MaxLevel);

			gl_Position = vec4(p.xy / float(sqrt(u_MaxLevel)), 0, 1);
			EmitVertex();
		}
		EndPrimitive();
	}
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) out vec4 o_FragColor;

void main()
{
	o_FragColor = vec4(1, 0, 0, 1);
}
#endif
