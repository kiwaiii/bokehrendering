//-----------------------------------------------------------------------------
#version 410
#extension GL_NV_gpu_shader5 : enable
#extension GL_EXT_shader_image_load_store : enable

//-----------------------------------------------------------------------------
layout(size1x32) coherent uniform uimage1D 	BokehCountTex;
layout(size4x32) coherent uniform  image2D 	BokehPositionTex;
layout(size4x32) coherent uniform  image2D 	BokehColorTex;
//-----------------------------------------------------------------------------
uniform sampler2D		PositionTex;
uniform sampler2D		RotationTex;
uniform sampler2D		InputTex;

uniform mat4			ViewMat;
uniform float			NearStart;
uniform float			NearEnd;
uniform float			FarStart;
uniform float			FarEnd;
uniform float			MaxRadius;
uniform int				nSamples;
uniform float			LumThreshold;
uniform float			CoCThreshold;
uniform float			AreaFactor;
uniform vec2 			Halton[32];
//------------------------------------------------------------------------------
in  vec2				TexCoord;
out vec4 				FragColor;

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define INV_PI			0.3183098861f
#define M_PI			3.141592654f

//------------------------------------------------------------------------------
// Halton sequence generated using: WONG, T.-T., LUK, W.-S., AND HENG, P.-A. 1997.
// Sampling with hammersley and Halton points
// http://www.cse.cuhk.edu.hk/~ttwong/papers/udpoint/udpoint.pdf
//------------------------------------------------------------------------------
float ExtractDepth(vec2 _pixelCoord)
{
	vec4 p		= textureLod(PositionTex,_pixelCoord,0);
	float atInf	= float(p.w==0.f);
	p.w			= 1.f;
	return max(-(ViewMat * p).z,atInf*1000.f);
}
//------------------------------------------------------------------------------
float ExtractBlur(float _depth)
{
	return clamp( (_depth-FarStart) / (FarEnd-FarStart), 0.f, 1.f);
}
//------------------------------------------------------------------------------
float saturate(float _value)
{
	return clamp(_value,0.f,1.f);
}
//------------------------------------------------------------------------------
void main()
{
	vec2 rcpSize= vec2(1.f) / vec2(textureSize(PositionTex,0));
	vec2 coord	= gl_FragCoord.xy * rcpSize;

	float depth = ExtractDepth(coord);
	float blur  = ExtractBlur(depth); 
	vec3  color = textureLod(InputTex,coord,0).xyz;
	float cocSize= blur * MaxRadius;

	vec2 angles = textureLod(RotationTex,coord,0).xy;
	mat2 rot 	= mat2(angles.x,angles.y,-angles.y,angles.x);

	vec3 avgColor = vec3(0);
	if(cocSize>0.f)
	{
		int count				= 1;
		vec3 outputColor		= color;
		float totalContribution	= 1;

		for(int i=0;i<nSamples;++i)
		{
			vec2  neighCoord	= coord + (rot*Halton[i]*rcpSize)*cocSize;
			float neighDepth	= ExtractDepth(neighCoord);
			float neighBlur		= ExtractBlur(neighDepth);
			vec3  neighColor	= textureLod(InputTex,neighCoord,0).xyz;
			float neighDist		= length(Halton[i])*cocSize;

			// Reject foreground samples, unless they're blurred as well
			float cocWeight		= saturate(cocSize + 1.0f - neighDist);
			float depthWeight	= float(neighDepth >= depth);
			float blurWeight	= neighBlur;
			float tapWeight		= cocWeight * saturate(depthWeight + blurWeight);

			outputColor			+= neighColor * tapWeight;
			totalContribution	+= tapWeight;
		}
		outputColor /= totalContribution;
		FragColor = vec4(outputColor,1);
avgColor = outputColor;
	}
	else
	{
		FragColor = vec4(color,1);
avgColor = color;
	}

	// Detect bokeh
	#if 0
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
		avgColor += textureLod(InputTex,(gl_FragCoord.xy+avgCoord[i])*rcpSize,0).xyz;
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
		vec3 c  = textureLod(InputTex,i,0).xyz;
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
	float avgLum     = dot(lumWeights, avgColor);
	float colorLum   = dot(lumWeights, color);
	float diffLum    = max(colorLum-avgLum,0);

	// Count point where intensity of neighbors is less than the current pixel
	if(diffLum>LumThreshold && cocSize>CoCThreshold)
	{
		ivec2 bufSize, coord;
		int current = int(imageAtomicAdd(BokehCountTex, 1, 1));
		bufSize 	= textureSize(InputTex,0).xy;
		coord.y 	= int(floor(current/bufSize.y));
		coord.x 	= current - coord.y*bufSize.y;
		vec3 lcolor = color.xyz / (M_PI*cocSize*cocSize*AreaFactor);
		imageStore(BokehPositionTex,coord,vec4(gl_FragCoord.x,gl_FragCoord.y,cocSize*1.5,0));
		imageStore(BokehColorTex,coord,vec4(lcolor,1));
	}

	// For screwing the compiler
	if(coord.x<-10000)
	{
		float value = NearStart + NearEnd + FarStart + FarEnd + MaxRadius + float(nSamples) + ViewMat[0].x + LumThreshold + CoCThreshold + AreaFactor;

		imageStore(BokehPositionTex,ivec2(0,0),vec4(0,0,0,0));
		imageStore(BokehColorTex,ivec2(0,0),vec4(1,0,0,1));
		imageAtomicAdd(BokehCountTex, 1, 1);

		FragColor   = vec4(value);
		return;
	}
}

