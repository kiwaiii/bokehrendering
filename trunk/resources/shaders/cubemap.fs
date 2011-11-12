#version 420 core

//-----------------------------------------------------------------------------
uniform sampler2DArray 	EnvTex;
//-----------------------------------------------------------------------------
in  vec3 				gTexCoord;
in  vec3 				gPosition;
out vec4 				FragColor;
//-----------------------------------------------------------------------------
void main()
{
    FragColor = texture(EnvTex,gTexCoord.xyz);
}
