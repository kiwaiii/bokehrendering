//-----------------------------------------------------------------------------
#version 410
#extension GL_NV_gpu_shader5 : enable
#extension GL_EXT_shader_image_load_store : enable
//-----------------------------------------------------------------------------
layout(size1x32) coherent uniform uimage1D 	BokehCountTex;
layout(size2x32) coherent uniform  image1D 	BokehPositionTex;
//------------------------------------------------------------------------------
uniform int									MaxBokehCount;
uniform sampler2D							InputTex;
//------------------------------------------------------------------------------
in  vec2									TexCoord;
out vec4 									FragColor;
//------------------------------------------------------------------------------
void main()
{
	ivec2 resolution = textureSize(InputTex,0);
	ivec2 pix		 = ivec2(gl_FragCoord.xy);
	vec3  value		 = texture(InputTex,TexCoord).xyz;
	float Coc		 = 20.f;

	if(value.x>0.5)
	{
		//int current = int(imageAtomicIncWrap(BokehCountTex, 0, 1));
		int current = int(imageAtomicAdd(BokehCountTex, 0, 1));
		if(current<MaxBokehCount)
			imageStore(BokehPositionTex,current,vec4(pix.x,pix.y,0,0));
//		imageStore(semaphoreImg, coords, uvec4(val ? 1U : 0U, 0U, 0U, 0U));

		FragColor = vec4(1,1,0,1);
		return;
	}


//	FragColor = texture(InputTex,ivec2(gl_FragCoor.xy)/vec2(resolution));
	FragColor = vec4(value,1);
//	FragColor = vec4(TexCoord,1,1);
}

