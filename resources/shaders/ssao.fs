//------------------------------------------------------------------------------
#version 410

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------
uniform sampler2D		PositionTex;
uniform sampler2D		NormalTex;
uniform sampler2D		RotationTex;

uniform float			Near;
uniform mat4			View;
uniform float			Beta;
uniform float			Epsilon;
uniform float			Kappa;
uniform float			Sigma;
uniform float			Radius;
uniform int				nSamples;
uniform vec2			Halton[32];

out vec4 				FragColor;

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define INV_PI			0.3183098861f
#define M_PI			3.141592654f


void main()
{
	vec2 pix	= gl_FragCoord.xy / vec2(textureSize(PositionTex,0));
	vec2 theta	= texture(RotationTex,pix).xy;
	vec4 c		= texture(PositionTex,pix);
	vec3 vn		= normalize( (View * vec4(texture(NormalTex,pix).xyz,0)).xyz );
 	vec3 vc		= (View * c).xyz;
	float r 	= Radius * abs(Near/vc.z);
	float A 	= 0;
	mat2 rot 	= mat2(theta.x,theta.y,-theta.y,theta.x);

	for(int i=0;i<nSamples;++i)
	{
		vec2 samp	= pix + (rot*Halton[i])*r;
		vec4 p		= texture(PositionTex,samp);
		vec3 v		= (View * p).xyz - vc;
		A 			+= max(0.f,dot(v,vn) + v.z*Beta)  / (dot(v,v) + Epsilon);
	}

	A 			= pow(max(0.f,1.f - 2.f*Sigma/float(nSamples)*A),Kappa);
	FragColor 	= vec4(A,A,A,A);
}

