//-----------------------------------------------------------------------------
#version 410

//-----------------------------------------------------------------------------
uniform sampler2D		BokehShapeTex;
uniform float			Attenuation;
//------------------------------------------------------------------------------
in  vec4				Radiance;
in  vec2				TexCoord;
out vec4 				FragColor;

//------------------------------------------------------------------------------
void main()
{
	// Add an attenuation function in order to avoid hard edge on the aperture 
	// shape
	float val = textureLod(BokehShapeTex,TexCoord,0).x;
	float att = clamp(length(2.f*(TexCoord-vec2(0.5))),0.f,1.f);
	att 	  = 1.f - pow(att,Attenuation);
	FragColor = vec4(Radiance.xyz * val * att,Radiance.w);
}

