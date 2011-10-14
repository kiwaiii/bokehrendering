//-----------------------------------------------------------------------------
#version 410

//-----------------------------------------------------------------------------
uniform vec3			SHLight[9];
uniform sampler2D		PositionTex;
uniform sampler2D		DiffuseTex;
uniform sampler2D		NormalTex;

out vec4 FragColor;

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define INV_PI          0.3183098861f
#define M_PI            3.1415926535f
//-----------------------------------------------------------------------------
const float 			c1 = 0.429043, 
						c2 = 0.511664, 
						c3 = 0.743125, 
						c4 = 0.886227, 
						c5 = 0.247708;
//-----------------------------------------------------------------------------
// Convolve with a clamped cos
// From Siggraph 02 An efficient representation for irradiance environment maps 
// [Ravi Ramamorthi, Pat Hanrahan]
// Compute the value of the kernel (including the scaling factor of the convolution)
void main()
{
	vec2 pix		= gl_FragCoord.xy / vec2(textureSize(PositionTex,0));
	vec3 d			= texture(DiffuseTex,pix).xyz;
	vec3 n			= normalize(texture(NormalTex,pix).xyz);
	vec3 radiance 	= 	c1 *  SHLight[8] * (n.x*n.x - n.y*n.y) 
					+	c3 *  SHLight[6] * n.z*n.z
					+	c4 *  SHLight[0]
					-	c5 *  SHLight[6] 
					+ 2*c1 * (SHLight[4]*n.x*n.y + SHLight[7]*n.x*n.z + SHLight[5]*n.y*n.z)
					+ 2*c2 * (SHLight[3]*n.x + SHLight[1]*n.y + SHLight[2]*n.z );

	FragColor	  	= vec4(radiance * d * INV_PI,1);
}
