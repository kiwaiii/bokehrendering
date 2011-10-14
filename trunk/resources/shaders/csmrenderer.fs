//-----------------------------------------------------------------------------
#version 410

//-----------------------------------------------------------------------------
uniform	int						nCascades;
uniform sampler2DArrayShadow	ShadowTex;
uniform vec2					HaltonPoints[32];
uniform sampler2D    			PositionTex;
uniform sampler2D    			DiffuseTex;
uniform sampler2D				NormalTex;

uniform vec3					LightDir;
uniform vec3					LightIntensity;
uniform mat4					LightViewProjs[4];

uniform float 					Bias;
uniform float					Aperture;
uniform int						nSamples;
//------------------------------------------------------------------------------
out vec4 						FragColor;

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define M_PI					3.141592654f
#define INV_PI					0.3183098861f
#define DISPLAY_CASCADES		0


//------------------------------------------------------------------------------
vec3 BRDF(	in vec3 _viewDir,
			in vec3 _lightDir,
			in vec3 _normal,
			in vec3 _diffuse,
			in float _roughness)
{
	vec3 h		= normalize(_viewDir+_lightDir);

	float F0	= 0.1f;
	float VdotH = max(1.f,dot(_viewDir,h));
	float NdotH = max(1.f,dot(_normal,h));
	float NdotV = max(1.f,dot(_normal,_viewDir));
	float NdotL = max(1.f,dot(_normal,_lightDir));

	float delta = acos(NdotH);

	float F		= F0 + (1.f-F0) * pow(1.f - VdotH,5.f);
	float D		= 1.f / (_roughness*_roughness * pow(cos(delta),4.f)) * exp(-pow(tan(delta)/_roughness, 2.f));
	float G		= min(1.f,min( 2.f*NdotH*NdotV/VdotH , 2.f*NdotH*NdotL/VdotH ));

	return _diffuse * D * G * F / (M_PI * NdotL * NdotV);
}
//------------------------------------------------------------------------------
float PCFShadow(const vec3 _pos, vec3 _d_dx, vec3 _d_dy, int _cascadeIndex, float _aperture, int _nSamples)
{
	// Compute the slope bias based on derivatives
	// Derivates represent variations of position in projective light space 
	// for a small variation in screen space
	// Inspired from : http://msdn.microsoft.com/en-us/library/ee416307(v=vs.85).aspx
	vec2 biasUV;
	biasUV.x 			= (_d_dy.y * _d_dx.z) - (_d_dx.y * _d_dy.z);
	biasUV.y 			= (_d_dx.x * _d_dy.z) - (_d_dy.x * _d_dx.z);
	biasUV     		   /= (_d_dx.x * _d_dy.y) - (_d_dx.y * _d_dy.x);

	// Compute the original bias (cst bias + slope bias for one texel)
	vec2 texel 			= 1.f/vec2(textureSize(ShadowTex,0));
	float slopeBias		= dot(abs(biasUV),texel);
	float defaultBias	= -slopeBias - Bias;

	// Compute the footprint of a screen space pixel into projective light space
	// multiple by the aperture
	vec2 vX				= vec2(_d_dx.x,_d_dy.x)*_aperture;
	vec2 vY				= vec2(_d_dx.y,_d_dy.y)*_aperture;
	
	float vis 			= 0;
	for(int i=0;i<_nSamples;++i)
	{
		// Compute the offset for the PCF
		vec2 delta	= HaltonPoints[i];
		delta 		= vec2(dot(delta,vX),dot(delta,vY));

		// Add the offset + the new bias for the offset and do the comparison
		vec3 samp	= _pos + vec3(delta, defaultBias + dot(biasUV,delta));
		vis 	   += texture(ShadowTex,vec4(samp.xy,_cascadeIndex,samp.z));
	}
	return vis / float(_nSamples);
}
//------------------------------------------------------------------------------
void main()
{
	// Get world position of the point to shade
	vec2 pix    	= gl_FragCoord.xy / vec2(textureSize(PositionTex,0));
	vec4 pos    	= textureLod(PositionTex,pix,0);
//	vec4 diffuse	= textureLod(DiffuseTex,pix,0);
//	float roughness = diffuse.w;
	vec3 diffuse	= textureLod(DiffuseTex,pix,0).xyz;
//	vec3 n			= normalize(cross(dFdx(pos.xyz),dFdy(pos.xyz)));

	// Select cascade
	// Compute derivates of position in projective light space for small 
	// variations in screen space
	// Face's normal (for light bump surface at grazing angle)
	vec3 lposs[4];
	vec3 d_dxs[4];
	vec3 d_dys[4];
	for(int i=0;i<nCascades;++i)
	{
		vec4 current	= LightViewProjs[i] * vec4(pos.xyz,1);
		current.xyz	   += vec3(1);
		current.xyz	   *= 0.5f;
		lposs[i]		= current.xyz;

		d_dxs[i]		= dFdx(current.xyz);
		d_dys[i]		= dFdy(current.xyz);
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
	float v		= PCFShadow(lposs[cindex].xyz, d_dxs[cindex], d_dys[cindex], cindex, Aperture, nSamples);
//	float rad	= max(0.f,dot(normalize(texture(NormalTex,pix).xyz),-LightDir)) * max(0.f,dot(n,-LightDir));
	float rad	= max(0.f,dot(normalize(texture(NormalTex,pix).xyz),-LightDir)) * INV_PI;
//	vec3 f		= BRDF(	viewDir,
//						-LightDir,
//						normal,
//						diffuse,
//						roughness);

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
	FragColor   = vec4(diffuse*LightIntensity*rad*v*color,1.f);
	#else
	FragColor   = vec4(diffuse*LightIntensity*rad*v,1.f);
//	FragColor   = vec4(LightIntensity*f*v,1.f);
	#endif

}

