#version 150
precision highp float;

in vec4 ciPosition;
in vec4 ciColor;
in mat4 aModelMatrix;

in vec3 centroidPosition;

uniform mat4 ciViewProjection;
uniform mat4 ciModelViewProjection;
uniform mat3 ciNormalMatrix;
uniform mat4 ciModelMatrix;

in vec3 ciNormal;
out highp vec3 Normal;

uniform vec3 cutPlane;
uniform float distance;
uniform vec4 boundingSphere;

out vec4 Color;
out int isVisible;

out VertexData {
	vec4 position;
	vec3 normal;
	vec4 color;
	vec2 texCoord;
} vVertexOut;

vec4 ClipPlane = vec4( cutPlane, distance );
void main() {
    gl_Position = ciViewProjection * aModelMatrix * ciPosition;
    //gl_Position = ciModelViewProjection * ciPosition;

    Color = ciColor;
    Normal = ciNormalMatrix * ciNormal;

    vVertexOut.position = ciViewProjection * aModelMatrix * ciPosition;
    vVertexOut.normal = ciNormalMatrix * ciNormal;
    //vVertexOut.color = ciColor;
    vVertexOut.color = vec4( 1, 0, 0, 1 );

    isVisible=1;
    vec3 distFromSphereCenter = centroidPosition - vec3( boundingSphere.x, boundingSphere.y, boundingSphere.z );
    float distSquared = dot( distFromSphereCenter, distFromSphereCenter );
    if( distSquared > ( boundingSphere.w * boundingSphere.w ) ) {
        isVisible = 0;
    }
    //if( dot(ciModelMatrix * vec4(centroidPosition,1), ClipPlane ) < 0.0f) {
    //    isVisible=0;
    //}
}
