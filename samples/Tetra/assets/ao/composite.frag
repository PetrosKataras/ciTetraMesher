#include "../common/offset.glsl"

uniform sampler2D uSampler;
uniform sampler2D uSamplerAo;

layout (location = 0) out vec4 oColor;

in vec2 uv;

void main( void )
{
	vec3 color	= texture( uSampler, uv ).rgb;
	color		-= 1.0 - texture( uSamplerAo, calcTexCoordFromUv( uv ) ).r;
	oColor		= vec4( color, 1.0 );
}
