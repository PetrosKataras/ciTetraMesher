#version 150
precision highp float;

out vec4 oFragColor;
in VertexData	{
	vec4 position;
	vec3 normal;
	vec4 color;
	vec2 texCoord;
} vVertexIn;

uniform vec3 lightPos;

in vec4 position;
void main() {

	// set diffuse and specular colors
	vec3 cDiffuse = vVertexIn.color.rgb;
	vec3 cSpecular = vec3( 0.3 );

	// light properties in view space
	vec3 vLightPosition = lightPos;

	// lighting calculations
	vec3 N = normalize( vVertexIn.normal );
	vec3 L = normalize( vLightPosition - vVertexIn.position.xyz );
	vec3 E = normalize( -vVertexIn.position.xyz );
	vec3 H = normalize( L + E );

	// Calculate coefficients.
	float phong = max( dot( N, L ), 0.0 );

	const float kMaterialShininess = 10.0;
	const float kNormalization = ( kMaterialShininess + 8.0 ) / ( 3.14159265 * 8.0 );
	float blinn = pow( max( dot( N, H ), 0.0 ), kMaterialShininess ) * kNormalization;

	// diffuse coefficient
	vec3 diffuse = vec3( phong );

	// specular coefficient 
	vec3 specular = blinn * cSpecular;

	// alpha 
	float alpha = 1.0;

	// final color
	oFragColor = vec4( diffuse + specular, alpha );
}
