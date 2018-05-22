
uniform mat4 u_ModelViewProjection;

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 i_Position;

void main()
{
	gl_Position = u_ModelViewProjection * vec4(i_Position, 1);
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) out vec4 o_FragColor;

void main()
{
	o_FragColor = vec4(1);
}
#endif
