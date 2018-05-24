
#ifdef VERTEX_SHADER
out vec2 uv;
void main()
{
	uv = vec2(gl_VertexID & 1, gl_VertexID >> 1 & 1);
	gl_Position = vec4(2.0 * uv - 1.0, 0.0, 1.0) ;
	//gl_Position.xy /= 1.05;
	gl_Position.x = 0.7 * gl_Position.x + 0.3;
}
#endif

#ifdef FRAGMENT_SHADER

in vec2 uv;
out vec4 color;

uniform sampler2D tex;

void main()
{
	color = texture(tex, uv);
}
#endif
