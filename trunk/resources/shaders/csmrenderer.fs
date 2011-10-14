//-----------------------------------------------------------------------------
#version 410

//-----------------------------------------------------------------------------
uniform	int						nCascades;
uniform sampler2DArrayShadow	ShadowTex;
uniform sampler2D    			PositionTex;
uniform sampler2D    			DiffuseTex;
uniform sampler2D				NormalTex;

uniform vec3					ViewPos;
uniform vec3					LightDir;
uniform vec3					LightIntensity;
uniform mat4					LightViewProjs[4];

uniform float					BlendFactor;
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
vec3 BRDF(	in vec3 _viewDir,
			in vec3 _lightDir,
			in vec3 _normal,
			in vec3 _diffuse,
			in float _roughness)
{
	vec3 h		= normalize(_viewDir+_lightDir);

	float F0	= 0.01f;
	float VdotH = max(0.f,dot(_viewDir,h));
	float NdotH = max(0.f,dot(_normal,h));
	float NdotV = max(0.f,dot(_normal,_viewDir));
	float NdotL = max(0.f,dot(_normal,_lightDir));

	float delta = acos(NdotH);

	float F		= F0 + (1.f-F0) * pow(1.f - VdotH,5.f);
	float D		= 1.f / (_roughness*_roughness * pow(cos(delta),4.f)) * exp(-pow(tan(delta)/_roughness, 2.f));
	float G		= min(1.f,min( 2.f*NdotH*NdotV/VdotH , 2.f*NdotH*NdotL/VdotH ));

//_diffuse * NdotL / M_PI + 
//	if(BlendFactor < 1000)
//		return ;
//	else
//		return ;
	return mix(_diffuse * NdotL / M_PI, _diffuse * D * G * F / (M_PI * NdotL * NdotV), BlendFactor);
}
//------------------------------------------------------------------------------
float ShadowTest(const vec3 _pos, int _cascadeIndex)
{
	return texture(ShadowTex,vec4(_pos.xy,_cascadeIndex,_pos.z-Bias));
}
//------------------------------------------------------------------------------
void main()
{
	// Get world position of the point to shade
	vec2 pix		= gl_FragCoord.xy / vec2(textureSize(PositionTex,0));
	vec4 pos		= textureLod(PositionTex,pix,0);
	vec3 n			= textureLod(NormalTex,pix,0).xyz;
	vec4 diffuse	= textureLod(DiffuseTex,pix,0);
	float roughness = diffuse.w;
	vec3 viewDir	= normalize(ViewPos-pos.xyz);

//if(roughness<0.1)
//{
//	FragColor   = vec4(10000.f,0.f,0.f,1.f);
//	return;
//}

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
//	float rad	= max(0.f,dot(normalize(texture(NormalTex,pix).xyz),-LightDir)) * INV_PI;
	vec3 f		= BRDF(viewDir,-LightDir,n,diffuse.xyz,roughness);

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
	FragColor   = vec4(f*LightIntensity*v,1.f);
	#endif

}

