//-----------------------------------------------------------------------------
#version 330 core

uniform sampler1D StarTex;
layout(location = ATTR_POSITION) in  vec3 	 Position;
out float vIntensity;

void main()
{
	vec4 data	= texelFetch(StarTex,gl_InstanceID,0);
	vIntensity	= data.w;
	gl_Position = vec4(data.xyz,1.f);
}

