#version 420 core

#ifdef GBUFFER
	uniform sampler2D   DiffuseTex;
	uniform sampler2D   NormalTex;

	in  vec3  ePosition;
	in  vec2  eTexCoord;

	layout(location = OUT_POSITION, 		index = 0) out vec4 FragPosition;
	layout(location = OUT_NORMAL_ROUGHNESS, index = 0) out vec4 FragNormal;
	layout(location = OUT_DIFFUSE_SPECULAR, index = 0) out vec4 FragDiffuse;

	void main()
	{
		FragPosition 	= vec4(ePosition,1);
		vec3 normal  	= textureLod(NormalTex,eTexCoord,0).xyz*2.f - 1.f;
		FragNormal		= vec4(normalize(normal),0.1);
		FragDiffuse		= vec4(texture(DiffuseTex,eTexCoord).xyz,1);
	}
#endif

#ifdef CSM_BUILDER
	void main()
	{

	}
#endif
