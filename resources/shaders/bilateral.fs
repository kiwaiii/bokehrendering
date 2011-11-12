#version 420 core

//-----------------------------------------------------------------------------
uniform sampler2D		InputTex;
uniform sampler2D		PositionTex;

uniform mat4			ViewMat;
uniform float			SigmaH;
uniform float			SigmaV;
uniform int				nTaps;
//------------------------------------------------------------------------------
out vec4 				FragColor;

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

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
	vec2 rcpSize = 1.f / vec2(textureSize(InputTex,0).xy);
	vec2 pix 	 = gl_FragCoord.xy * rcpSize;
	vec2 offsetX = vec2(1,0) * rcpSize;
	vec2 offsetY = vec2(0,1) * rcpSize;
	vec4 color	 = vec4(0);
	float totalW = 0;

	vec4 dref	 = ViewMat * vec4(textureLod(PositionTex,pix,0).xyz,1);
	vec4 cref 	 = textureLod(InputTex, pix, 0);

	// We average the alpha channel since we use it for blending SSAO
	for(int j=-nTaps;j<=nTaps;++j)
	for(int i=-nTaps;i<=nTaps;++i)
	{
		vec2 crd = pix + i*offsetX + j*i*offsetY;
		vec4 d	 = ViewMat * vec4(textureLod(PositionTex,crd,0).xyz,1);
		vec4 c	 = textureLod(InputTex,crd,0);
		float w  = GaussianWeight(float(i),SigmaH) * GaussianWeight(abs(d.z-dref.z),SigmaV);
		color 	+= w  * c;
		totalW  += w;
	}
	color /= totalW;

	FragColor 	= color;
}

