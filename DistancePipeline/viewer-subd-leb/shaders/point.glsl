
uniform vec2 u_Target;

#ifdef VERTEX_SHADER
void main()
{
	gl_Position = vec4(2 * u_Target - 1, 0, 1);
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) out vec4 o_FragColor;

void main()
{
	vec2 P = vec2(gl_PointCoord.x, 1.0 - gl_PointCoord.y);
	vec2 p = 2.0 * P - 1.0;
	float r = dot(p, p);
	if (r > 1.0)
			discard;

	o_FragColor = vec4(0);
}
#endif
