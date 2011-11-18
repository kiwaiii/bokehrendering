#version 420 core

//-----------------------------------------------------------------------------
uniform	int						nCascades;
uniform sampler2DArrayShadow	ShadowTex;
uniform sampler2D				PositionTex;
uniform sampler2D				DiffuseTex;
uniform sampler2D				NormalTex;

uniform vec3					ViewPos;
uniform vec3					LightDir;
uniform vec3					LightIntensity;
uniform mat4					LightViewProjs[4];

uniform float					BlendFactor;	// Fake variable
uniform float 					Bias;
//------------------------------------------------------------------------------
out vec4 						FragColor;

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define M_PI					3.141592654f
#define INV_PI					0.3183098861f
#define DISPLAY_CASCADES		0


//------------------------------------------------------------------------------
vec3 CTBRDF(in vec3 _viewDir,
			in vec3 _lightDir,
			in vec3 _normal,
			in vec3 _diffuse,
			in float _roughness,
			in float _specularity)
{
	// TODO : Optimize thisone. There is a large room for optimizating ...
	// Remove BlendFactor : it is here for debugging

	vec3  h		= normalize(_viewDir+_lightDir);
	float F0	= 0.1f;
	float VdotH = max(0.0f,dot(_viewDir,h));
	float NdotH = max(0.0f,dot(_normal,h));
	float NdotL = max(0.0f,dot(_normal,_lightDir));
	float NdotV = max(0.0f,dot(_normal,_viewDir));
	float sNdotH= sqrt(1.f-NdotH*NdotH);

	// Basic Cook-Torrance
	float kappa	= sNdotH/(NdotH*_roughness);
	float F		= F0 + (1.f-F0) * pow(1.f - VdotH,5.f);
	float D		= 1.f / (M_PI * _roughness*_roughness * pow(NdotH,4.f)) * exp(-kappa*kappa);
	float G		= min(1.f,min( 2.f*NdotH*NdotV/VdotH , 2.f*NdotH*NdotL/VdotH ));

	// Max function prevents "nan" value due to the denominator
	float val	= max(0.f,D * F *G / (NdotL * NdotV));


	if(gl_FragCoord.x < 10000)
//		return vec3(max(0,dot(_normal,_lightDir)));
//		return vec3(max(0,dot(normalize(_normal),normalize(_lightDir))));
//		return vec3(NdotL / M_PI + _specularity*val);
		return vec3(val);
	else
		return _diffuse * (NdotL / M_PI + BlendFactor*_specularity*val);
}
//------------------------------------------------------------------------------
vec3 WangBRDF(	in vec3  _viewDir,
				in vec3  _lightDir,
				in vec3  _normal,
				in vec3  _diffuse,
				in float _roughness,
				in float _specularity)
{
	// Use Wang09 approximation for CT BRDF
	vec3  h			= normalize(_viewDir+_lightDir);
	float VdotH 	= max(0.0f,dot(_viewDir,h));
	float NdotV 	= max(0.0f,dot(_normal,_viewDir));

	// Use schlick approximation for Fresnel
	// Use Kelemen and Szirmau-Kalos apprixmation for the geometric term
	float F0		= 0.1f;
	float F			= F0 + (1.f-F0) * pow(1.f - VdotH,5.f);
	float M0		= max(0.f,F / (VdotH * VdotH));

	// Wrap NDF to the explicit BRDF response (assume isotropic ouput)
	float lambda_d	= 2.f/(_roughness*_roughness) - 2;
	float lambda_w	= clamp(lambda_d / float(abs(4*NdotV)),0.f,20000.f);
	vec3 p_w		= normalize(2.f * NdotV * _normal - _viewDir);
	float D			= (lambda_d+2) / (2*M_PI) * exp(lambda_w * (dot(p_w,_lightDir)-1) );
	float val		= M0 * D;

	return vec3(val);
}
//------------------------------------------------------------------------------
float ShadowTest(const vec3 _pos, int _cascadeIndex)
{
	// Basic shadow test
	// TODO : re-add PCF and EVSM
	return texture(ShadowTex,vec4(_pos.xy,_cascadeIndex,_pos.z-Bias));
}
//------------------------------------------------------------------------------
void main()
{
	// Get world position of the point to shade
	vec2 pix			= gl_FragCoord.xy / vec2(textureSize(PositionTex,0));
	vec4 pos			= textureLod(PositionTex,pix,0);
	vec4 normal			= textureLod(NormalTex,pix,0);
	vec4 diffuse		= textureLod(DiffuseTex,pix,0);
	float roughness		= normal.w;
	float specularity	= diffuse.w;
	vec3 viewDir		= normalize(ViewPos-pos.xyz);

	// Select cascade
	// Compute derivates of position in projective light space for small 
	// variations in screen space
	vec3 lposs[4];
	for(int i=0;i<nCascades;++i)
	{
		vec4 current	= LightViewProjs[i] * vec4(pos.xyz,1);
		current.xyz	   += vec3(1);
		current.xyz	   *= 0.5f;
		lposs[i]		= current.xyz;
	}

	// Select cascade
	int cindex = 0;
	for(;cindex<nCascades;++cindex)
	{
		vec2 test1		= vec2(greaterThanEqual(lposs[cindex].xy,vec2(0,0)));
		vec2 test2		= vec2(lessThanEqual(lposs[cindex].xy,vec2(1,1)));

		if(int(dot(test1,test1)+dot(test2,test2))==4)
			break;
	}

	// Compute radiance
	float v		= ShadowTest(lposs[cindex].xyz, cindex);
//	vec3 f		= BRDF(viewDir,-LightDir,normal.xyz,diffuse.xyz,roughness,specularity);

	vec3 f		= mix(	CTBRDF(viewDir,-LightDir,normal.xyz,diffuse.xyz,roughness,specularity),
						WangBRDF(viewDir,-LightDir,normal.xyz,diffuse.xyz,roughness,specularity),
						BlendFactor);


	#if DISPLAY_CASCADES
	vec3 color;
	{
			 if(cindex==0)
				color = vec3(1.f,0.f,0.f); 
		else if(cindex==1)
				color = vec3(0.f,1.f,0.f); 
		else if(cindex==2)
				color = vec3(0.f,0.f,1.f); 
		else if(cindex==3)
				color = vec3(1.f,1.f,0.f); 
		else 
				color = vec3(1.f,0.f,1.f);
	}
	FragColor   = vec4(f*LightIntensity*v*color,1.f);
	#else
	FragColor   = vec4(f*LightIntensity*v,1.f);
	#endif
}

