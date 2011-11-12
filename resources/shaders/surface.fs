#version 330

uniform sampler2D   Texture;
//uniform vec2        FrameSize;
uniform float       Level;
out vec4            FragColor;

void main()
{
	FragColor = textureLod(Texture, gl_FragCoord.xy * vec2(RCP_SCREEN_X,RCP_SCREEN_Y), Level);
}
