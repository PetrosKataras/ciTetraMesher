#version 150

layout( triangles ) in;
layout( triangle_strip ) out;
layout( max_vertices = 3 ) out;

in vec3 Normal[];
in vec4 Color[];
in int isVisible[];

out vec3 gNormal;
out vec4 gColor;
out vec4 position;

out VertexData {
	vec4 position;
	vec3 normal;
	vec4 color;
	vec2 texCoord;
}vVertexOut;

vec3 GetNormal()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return normalize(cross(a, b));
}  

vec4 plane = vec4(0, 1, 0, 7);
void main()
{
    gNormal = Normal[0];
    gColor = Color[0];
    vVertexOut.normal = GetNormal();
    
    for( int i = 0; i < 3; i++ ) {
        if( isVisible[i] == 1 ) {
            gl_Position = gl_in[i].gl_Position;
            vVertexOut.position = gl_in[i].gl_Position;
            vVertexOut.color = Color[i];
            EmitVertex();
        }
    }
    EndPrimitive();
}
