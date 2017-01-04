#version 330
precision highp float;

in vec4 ciPosition;
in vec4 ciColor;
in vec3 ciNormal;

in vec3 centroidPosition;

uniform mat4 ciViewProjection;
uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
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
    gl_Position = ciModelViewProjection * ciPosition;
    vVertexOut.position = ciModelView * ciPosition;
    vVertexOut.normal = ciNormalMatrix * ciNormal;
    vVertexOut.color = ciColor;

    vec3 distFromSphereCenter = centroidPosition - boundingSphere.xyz;
    float distSquared = dot( distFromSphereCenter, distFromSphereCenter );
    vVertexOut.isVisible = distSquared > ( boundingSphere.w * boundingSphere.w ) ? 0 : 1;
}
