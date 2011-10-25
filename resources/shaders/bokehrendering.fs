//-----------------------------------------------------------------------------
#version 410

//-----------------------------------------------------------------------------
uniform sampler2D		BokehShapeTex;
uniform sampler2D		BlurDepthTex;
uniform float			Attenuation;
//------------------------------------------------------------------------------
in  vec4				gColor;
in  float				gDepth;
in  vec2				gTexCoord;
out vec4 				FragColor;

//------------------------------------------------------------------------------
void main()
{
	// Add an attenuation function in order to avoid hard edge on the aperture 
	// shape
	float val	= textureLod(BokehShapeTex,gTexCoord,0).x;
	float att	= clamp(length(2.f*(gTexCoord-vec2(0.5))),0.f,1.f);
	att 		= 1.f - pow(att,Attenuation);
	vec2 bd		= textureLod(BlurDepthTex,gl_FragCoord.xy/vec2(textureSize(BlurDepthTex,0)),0).xy;
	float blur  = bd.x;
	float depth = bd.y;

//	float below = float(gDepth<=depth) * float(blur<0.1);
	float weight= 1.f;
	if(gDepth<=depth)
	{
//		if(blur<1.0)
////			weight = 0.f;
//			FragColor = vec4(15000,0,0,1);
//		else
//			FragColor = vec4(0,15000,0,1);
			FragColor = vec4(15000*blur,0,0,1);
		return;
	}

	FragColor = vec4(gColor.xyz * val * att * weight,gColor.w);

//	if(gTexCoord.x < 100000 && gDepth>=depth)
//	if(gTexCoord.x < 100000)
//	{
//		if(gDepth<=depth)
//			FragColor = vec4(15000,0,0,1);
//		else
//			FragColor = vec4(0,15000,0,1);
//	}
//	float below = float(gDepth>=depth);// * float(bd.y<0.1);
}

