//-----------------------------------------------------------------------------
#version 410
#extension GL_NV_gpu_shader5 : enable
#extension GL_EXT_shader_image_load_store : enable

//-----------------------------------------------------------------------------
layout(size1x32) coherent uniform uimage1D 	CountTex;
layout(size4x32) coherent uniform  image2D 	SampleTex;
layout(size4x32) coherent uniform  image2D 	ColorTex;
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
vec2 Halton[32];

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
	Halton[0]	= vec2(-0.353553, 0.612372);
	Halton[1]	= vec2(-0.25, -0.433013);
	Halton[2]	= vec2(0.663414, 0.55667);
	Halton[3]	= vec2(-0.332232, 0.120922);
	Halton[4]	= vec2(0.137281, -0.778559);
	Halton[5]	= vec2(0.106337, 0.603069);
	Halton[6]	= vec2(-0.879002, -0.319931);
	Halton[7]	= vec2(0.191511, -0.160697);
	Halton[8]	= vec2(0.729784, 0.172962);
	Halton[9]	= vec2(-0.383621, 0.406614);
	Halton[10]	= vec2(-0.258521, -0.86352);
	Halton[11]	= vec2(0.258577, 0.34733);
	Halton[12]	= vec2(-0.82355, 0.0962588);
	Halton[13]	= vec2(0.261982, -0.607343);
	Halton[14]	= vec2(-0.0562987, 0.966608);
	Halton[15]	= vec2(-0.147695, -0.0971404);
	Halton[16]	= vec2(0.651341, -0.327115);
	Halton[17]	= vec2(0.47392, 0.238012);
	Halton[18]	= vec2(-0.738474, 0.485702);
	Halton[19]	= vec2(-0.0229837, -0.394616);
	Halton[20]	= vec2(0.320861, 0.74384);
	Halton[21]	= vec2(-0.633068, -0.0739953);
	Halton[22]	= vec2(0.568478, -0.763598);
	Halton[23]	= vec2(-0.0878153, 0.293323);
	Halton[24]	= vec2(-0.528785, -0.560479);
	Halton[25]	= vec2(0.570498, -0.13521);
	Halton[26]	= vec2(0.915797, 0.0711813);
	Halton[27]	= vec2(-0.264538, 0.385706);
	Halton[28]	= vec2(-0.365725, -0.76485);
	Halton[29]	= vec2(0.488794, 0.479406);
	Halton[30]	= vec2(-0.948199, 0.263949);
	Halton[31]	= vec2(0.0311802, -0.121049);


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



	// Count point where intensity of neighbors is less than the current pixel
	float lumNeighs = dot(cneighs, vec3(0.299f, 0.587f, 0.114f));
	float lumCenter = dot(ccenter, vec3(0.299f, 0.587f, 0.114f));
	if((lumCenter-lumNeighs)>IntThreshold && r>CoCThreshold)
	{
		ivec2 bufSize, coord;
		int current = int(imageAtomicAdd(CountTex, 1, 1));
		bufSize 	= textureSize(InputTex,0).xy;
		coord.y 	= int(floor(current/bufSize.y));
		coord.x 	= current - coord.y*bufSize.y;
		vec3 lcolor = ccenter.xyz / (M_PI*r*r*AreaFactor);

		imageStore(SampleTex,coord,vec4(gl_FragCoord.x,gl_FragCoord.y,r,0));
		imageStore(ColorTex,coord,vec4(lcolor,1));
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
