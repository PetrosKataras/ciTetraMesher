#version 150
precision highp float;

//in vec4 gColor;
//out vec4 oColor;
//in vec3 gNormal;
const int uTexturingMode = 3;
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
    //const vec3 L = vec3( 0, 0, 1 );
    //vec3 N = normalize( gNormal );
    //float lambert = max( 0.0, dot( N, L ) );
    //oColor = vec4(1) * gColor * vec4( vec3( lambert ), 1.0 );

	// set diffuse and specular colors
	vec3 cDiffuse = vVertexIn.color.rgb;
	vec3 cSpecular = vec3( 0.3 );

	// light properties in view space
	vec3 vLightPosition = lightPos;// vec3( 0.0, 1.0, 1.0 );

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

	//if( uTexturingMode == 1 ) {
	//	diffuse *= vec3( 0.7, 0.5, 0.3 );
	//	diffuse *= 0.5 + 0.5 * checkered( vVertexIn.texCoord, uFreq );
	//}
	//else if ( uTexturingMode == 2 )
	//	diffuse *= texture( uTex0, vVertexIn.texCoord.st ).rgb;

    //diffuse *= vec3( 0.7, 0.5, 0.3 );
	// specular coefficient 
	vec3 specular = blinn * cSpecular;

	// alpha 
	float alpha = ( uTexturingMode == 4 ) ? 0.75 : 1.0;

	// final color
	oFragColor = vec4( diffuse + specular, alpha );
}
