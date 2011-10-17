//-----------------------------------------------------------------------------
#version 410
#extension GL_NV_gpu_shader5 : enable
#extension GL_EXT_shader_image_load_store : enable

//-----------------------------------------------------------------------------
layout(size1x32) coherent uniform uimage1D 	BokehCountTex;
layout(size4x32) coherent uniform  image2D 	BokehPosTex;
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
uniform float			IntThreshold;
uniform float			CoCThreshold;
uniform float			AreaFactor;
uniform vec2 			Halton[32];
//------------------------------------------------------------------------------
out vec4 				FragColor;

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define INV_PI          0.3183098861f
#define M_PI			3.141592654f

//------------------------------------------------------------------------------
// Halton sequence generated using: WONG, T.-T., LUK, W.-S., AND HENG, P.-A. 1997.Sampling with hammersley and Halton points
// http://www.cse.cuhk.edu.hk/~ttwong/papers/udpoint/udpoint.pdf

//------------------------------------------------------------------------------
float GaussianWeight(float _s, float _sigma)
{
	float twoSigma2 = 2.f * _sigma * _sigma;
	float factor = 1.f / sqrt(3.141592654f * twoSigma2);
	return factor * exp(-(_s * _s) / twoSigma2);
}
//------------------------------------------------------------------------------
void main()
{
	vec2 rcpSize= vec2(1.f) / vec2(textureSize(PositionTex,0));
	vec2 pix	= gl_FragCoord.xy * rcpSize;
	vec4 pref	= textureLod(PositionTex,pix,0);
	float atInf	= float(pref.w==0.f);
	pref.w		= 1.f;
 	float zref	= max(-(ViewMat * pref).z,atInf*1000.f);
	float r 	= clamp( MaxRadius * (zref-FarStart) / (FarEnd-FarStart), 0.f, MaxRadius);
	vec2 angles = textureLod(RotationTex,pix,0).xy;
	mat2 rot 	= mat2(angles.x,angles.y,-angles.y,angles.x);
	vec3 color	= textureLod(InputTex,pix,0).xyz;
	vec3 ccenter= color;
	vec3 cneighs= vec3(0);


	int count	= 1;
	for(int i=0;i<nSamples;++i)
	{
		vec2 samp	= pix + (rot*Halton[i]*rcpSize)*r;
		vec4 p		= textureLod(PositionTex,samp,0);
		atInf		= float(p.w==0.f);
//		p.z			= 1.f;
		float z		= max(-(ViewMat * p).z,atInf*1000.f);
		vec3 c		= textureLod(InputTex,samp,0).xyz;
		int	toAdd	= int(z>=FarStart*0.8);
		color		+= c*float(toAdd);
		cneighs		+= c*float(toAdd);
		count		+= toAdd;
	}
	color /= count;
	cneighs/= (count==1?1:count-1);

	// Count point where intensity of neighbors is less than the current pixel
	float lumNeighs = dot(cneighs, vec3(0.299f, 0.587f, 0.114f));
	float lumCenter = dot(ccenter, vec3(0.299f, 0.587f, 0.114f));
	if((lumCenter-lumNeighs)>IntThreshold && r>CoCThreshold)
	{
		ivec2 bufSize, coord;
		int current = int(imageAtomicAdd(BokehCountTex, 1, 1));
		bufSize 	= textureSize(InputTex,0).xy;
		coord.y 	= int(floor(current/bufSize.y));
		coord.x 	= current - coord.y*bufSize.y;
		vec3 lcolor = ccenter.xyz / (M_PI*r*r*AreaFactor);

		imageStore(BokehPosTex,coord,vec4(gl_FragCoord.x,gl_FragCoord.y,r,0));
		imageStore(BokehColorTex,coord,vec4(lcolor,1));
	}
	FragColor 	= vec4(color,1);


	// To screw the compiler
	if(pix.x<-10000)
	{
		float value = NearStart + NearEnd + FarStart + FarEnd + MaxRadius + float(nSamples) + ViewMat[0].x;
		FragColor   = vec4(value);
		return;
	}
}

//	int count	= 1;
//	for(int i=0;i<nSamples;++i)
//	{
//		vec2 samp	= pix + (rot*Halton[i]*rcpSize)*r;
//		vec4 p		= textureLod(PositionTex,samp,0);
//		atInf		= float(p.w==0.f);
//		float z		= max(-(ViewMat * p).z,atInf*1000.f);
//		vec3 c		= textureLod(InputTex,samp,0).xyz;
//		int	toAdd	= int(z>=FarStart*0.8);
//		color		+= c*float(toAdd);
//		count		+= toAdd;
//	}
//	color /= count;


//	cneighs		+= color;
//	for(int i=-2;i<=2;++i)
//	for(int j=-2;j<=2;++j)
//	{
//		vec2 samp	= pix + vec2(i,j)*rcpSize;
//		vec4 p		= textureLod(PositionTex,samp,0);
//		atInf		= float(p.w==0.f);
//		float z		= max(-(ViewMat * p).z,atInf*1000.f);
//		vec3 c		= textureLod(InputTex,samp,0).xyz;
//		int	toAdd	= int(z>=FarStart*0.8);
//		cneighs		+= c*float(toAdd);
//		count		+= toAdd;
//		cneighs		+= c;
//	}
//	cneighs/= (count==1?1:count-1);
//	cneighs/= 25;

