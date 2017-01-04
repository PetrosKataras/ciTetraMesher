#version 330
precision highp float;

out vec4 oFragColor;
in VertexData	{
	vec4 position;
	vec3 normal;
	vec4 color;
	vec2 texCoord;
} gVertexIn;

in vec4 gPosition;

#ifdef ENABLE_GBUFFER_PASS
    layout (location = 0) out vec4 oAlbedo;
    layout (location = 1) out vec4 oNormalEmissive;
    layout (location = 2) out vec4 oPosition;
    uniform float uEmissive;
#endif

void main() {
#ifndef ENABLE_GBUFFER_PASS
        // set diffuse and specular colors
        vec3 cDiffuse = gVertexIn.color.rgb;
        vec3 cSpecular = vec3( 0.3 );

        // light properties in view space
        vec3 vLightPosition = vec3( 1.0, 1.0, 1.0 );

        // lighting calculations
        vec3 N = normalize( gVertexIn.normal );
        vec3 L = normalize( vLightPosition - gVertexIn.position.xyz );
        vec3 E = normalize( -gVertexIn.position.xyz );
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
#else
        oAlbedo 		= gVertexIn.color;
        oNormalEmissive	= vec4( normalize( gVertexIn.normal ), uEmissive );
        oPosition 		= gPosition;
#endif
}
