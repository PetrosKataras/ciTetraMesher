#version 150
precision highp float;

in vec4 ciPosition;
in vec4 ciColor;
in vec3 ciNormal;

in mat4 aModelMatrix;
in vec3 centroidPosition;

uniform mat4 ciViewProjection;
uniform mat4 ciModelViewProjection;
uniform mat3 ciNormalMatrix;
uniform mat4 ciModelMatrix;

uniform float distance;
uniform vec4 boundingSphere;

out VertexData {
	vec4 position;
	vec3 normal;
	vec4 color;
	vec2 texCoord;
    int isVisible;
} vVertexOut;

void main() {
    gl_Position = ciViewProjection * aModelMatrix * ciPosition;

    vVertexOut.position = gl_Position;
    vVertexOut.normal = ciNormalMatrix * ciNormal;
    vVertexOut.color = ciColor;

    vec3 distFromSphereCenter = centroidPosition - vec3( boundingSphere.x, boundingSphere.y, boundingSphere.z );
    float distSquared = dot( distFromSphereCenter, distFromSphereCenter );
    vVertexOut.isVisible = distSquared > ( boundingSphere.w * boundingSphere.w ) ? 0 : 1;
}
