//-----------------------------------------------------------------------------
#version 410
#extension GL_NV_gpu_shader5 : enable
#extension GL_EXT_shader_image_load_store : enable

//-----------------------------------------------------------------------------
layout(size1x32) coherent uniform uimage1D 	BokehCountTex;
layout(size4x32) coherent uniform  image2D 	BokehPositionTex;
layout(size4x32) coherent uniform  image2D 	BokehColorTex;
//-----------------------------------------------------------------------------
uniform sampler2D		BlurDepthTex;
uniform sampler2D		ColorTex;
uniform float			MaxCoCRadius;
uniform float			LumThreshold;
uniform float			CoCThreshold;
out vec4 				FragColor;
//------------------------------------------------------------------------------


void main()
{
	vec2 rcpSize  =  1.f / vec2(textureSize(ColorTex,0));
	vec2  coord   = gl_FragCoord.xy * rcpSize;
	vec2  bd	  = textureLod(BlurDepthTex,coord,0).xy;
	float blur    = bd.x;
	float depth   = bd.y;
	vec3  color   = textureLod(ColorTex,coord,0).xyz;
	float cocSize = blur * MaxCoCRadius;

	// Compute neighborhood color
	#if 1
	vec3 avgColor   = vec3(0);
	vec2 avgCoord[9];
	avgCoord[0] = vec2(-1.5f, -1.5f);
	avgCoord[1] = vec2(0.5f, -1.5f);
	avgCoord[2] = vec2(1.5f, -1.5f);
	avgCoord[3] = vec2(-1.5f, 0.5f);
	avgCoord[4] = vec2(0.5f, 0.5f);
	avgCoord[5] = vec2(1.5f, 0.5f);
	avgCoord[6] = vec2(-1.5f, 1.5f);
	avgCoord[7] = vec2(0.5f, 1.5f);
	avgCoord[8] = vec2(1.5f, 1.5f);
	for(int i=0;i<9;++i)
	{
		avgColor += textureLod(ColorTex,(gl_FragCoord.xy+avgCoord[i])*rcpSize,0).xyz;
	}
	avgColor /= 9;
	#endif

	#if 0
	int count       = 0;
	vec3 avgColor   = vec3(0);
	for(int y=-2;y<2;++y)
	for(int x=-2;x<2;++x)
	{
		vec2 i  = (gl_FragCoord.xy+vec2(0.5f+0.5f*x,0.5f+0.5f*y))*rcpSize;
		vec3 c  = textureLod(ColorTex,i,0).xyz;
		float d = ExtractDepth(i);
		if(d>depth)
		{
			avgColor += c;
			++count;
		}
	}
	avgColor /= max(count,1);
	#endif

	// Compute luminosity
//	vec3  lumWeights = vec3(0.299f, 0.587f, 0.114f);
	vec3  lumWeights = vec3(1, 1, 1);
	float colorLum   = dot(lumWeights, color);
	float avgLum     = dot(lumWeights, avgColor);
	float difLum     = max(colorLum-avgLum,0);

	// Count point where intensity of neighbors is less than the current pixel
	if(difLum>LumThreshold && cocSize>CoCThreshold)
	{
		ivec2 bufSize, coord;
		int current = int(imageAtomicAdd(BokehCountTex, 1, 1));
		bufSize 	= textureSize(ColorTex,0).xy;
		coord.y 	= int(floor(current/bufSize.y));
		coord.x 	= current - coord.y*bufSize.y;
		vec3 lcolor = color.xyz / (3.141592654f*cocSize*cocSize*1.f);
		imageStore(BokehPositionTex,coord,vec4(gl_FragCoord.x,gl_FragCoord.y,depth,blur));
		imageStore(BokehColorTex,coord,vec4(lcolor,1));
		color 		= vec3(0);
	}

	FragColor = vec4(color,1);
}

