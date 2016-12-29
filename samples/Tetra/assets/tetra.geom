#version 150

layout( triangles ) in;
layout( triangle_strip ) out;
layout( max_vertices = 3 ) out;

in VertexData {
    vec4 position;
    vec3 normal;
    vec4 color;
    vec2 texCoord;
    int isVisible;
}gVertexIn[];

out VertexData {
	vec4 position;
	vec3 normal;
	vec4 color;
	vec2 texCoord;
}gVertexOut;

vec3 GetNormal()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return normalize(cross(a, b));
}  

void main()
{
    gVertexOut.normal = GetNormal();
    for( int i = 0; i < 3; i++ ) {
        if( gVertexIn[i].isVisible == 1 ) {
            gl_Position = gl_in[i].gl_Position;
            gVertexOut.position = gl_in[i].gl_Position;
            gVertexOut.color = gVertexIn[i].color;
            EmitVertex();
        }
    }
    EndPrimitive();
}
