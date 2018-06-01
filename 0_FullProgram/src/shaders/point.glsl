#line 1

#ifdef VERTEX_SHADER

uniform vec2 pos2D;

vec4 toScreenSpace(vec2 v)
{
    vec2 tmp = v;
    return vec4(tmp.x, tmp.y, 0 , 1);
}

void main(){
    gl_Position = toScreenSpace(pos2D);
}

#endif

#ifdef FRAGMENT_SHADER
out vec4 color;
void main()
{
    color = vec4(1,0,0,1);
}

#endif
