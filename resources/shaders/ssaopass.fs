//-----------------------------------------------------------------------------
#version 410

//-----------------------------------------------------------------------------
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

//	FragColor 	= vec4(vec3(  acos(theta.x)/M_PI  ),1);
//	FragColor 	= vec4(vec3(  (theta.x+1)*0.5  ),1);
//	FragColor	= vec4(r,r,r,1);
//	FragColor	= vec4(vc,1);

	// To screw the compiler
	if(c.x<-10000)
	{
		float value = Beta * Epsilon * Kappa * Sigma * Radius * nSamples + texture(PositionTex,pix).x + texture(NormalTex,pix).x * View[0].x + Near;
		FragColor   = vec4(value);
		return;
	}
}

