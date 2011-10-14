//-----------------------------------------------------------------------------
//#version 330 core
#version 410 core

uniform sampler2D InputTex;
out vec4 FragColor;

void main()
{
	vec2 pix 	= gl_FragCoord.xy/vec2(textureSize(InputTex,0));
	vec3 color 	= textureLod(InputTex,pix,0).xyz;
	float lum	= max(dot(color, vec3(0.299f, 0.587f, 0.114f)), 0.0001f);
	FragColor 	= vec4(lum,lum,lum,1);
}
