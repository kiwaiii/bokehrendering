#version 420 core

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define INV_PI          0.3183098861f
#define M_PI            3.1415926535f

#ifdef BUILDER
	uniform samplerCube  CubeTex;
	uniform mat4         Transformations[6];

	layout(location = OUT_COEFF0, index = 0) out vec4 SHCoeffs0;
	layout(location = OUT_COEFF1, index = 0) out vec4 SHCoeffs1;
	layout(location = OUT_COEFF2, index = 0) out vec4 SHCoeffs2;
	layout(location = OUT_COEFF3, index = 0) out vec4 SHCoeffs3;
	layout(location = OUT_COEFF4, index = 0) out vec4 SHCoeffs4;
	layout(location = OUT_COEFF5, index = 0) out vec4 SHCoeffs5;
	layout(location = OUT_COEFF6, index = 0) out vec4 SHCoeffs6;

	void main()
	{
		// Compute solid angle weight
		// Each face has a 2x2 unit surface placed at 1 unit from the center
		// Thus the area of a face is 4
		// The solid angle is :
		// \Omega = cos\theta / r2 * texelArea
		// cos\theta is equal to 1/sqrt(r2)	
		ivec2 texSize	= textureSize(CubeTex,0).xy;
		float texArea	= 4.f/float(texSize.x*texSize.y); 
		vec2  texCoord  = (gl_FragCoord.xy) / vec2(texSize);
		vec2  texelPos  = (gl_FragCoord.xy + vec2(0.5f)) / vec2(texSize)*2.f - vec2(1.f);
		float r2	    = texelPos.x*texelPos.x + texelPos.y*texelPos.y + 1;
		float weight    = 1.f / (sqrt(r2)*r2) * texArea;
		vec3 refDir     = normalize(vec3(texelPos,-1));

		vec3 SHCoeffs[9];
		SHCoeffs[0] = vec3(0);
		SHCoeffs[1] = vec3(0);
		SHCoeffs[2] = vec3(0);
		SHCoeffs[3] = vec3(0);
		SHCoeffs[4] = vec3(0);
		SHCoeffs[5] = vec3(0);
		SHCoeffs[6] = vec3(0);
		SHCoeffs[7] = vec3(0);
		SHCoeffs[8] = vec3(0);

		for(int l=0;l<6;++l)
		{
			vec3 dir	 = (Transformations[l] * vec4(refDir,0)).xyz;
			vec3 value   = weight * textureLod(CubeTex,dir,0).xyz;

			// Compute SH function
			SHCoeffs[0] += value * 0.282095;
			SHCoeffs[1] += value * 0.488603 *  dir.y;
			SHCoeffs[2] += value * 0.488603 *  dir.z;
			SHCoeffs[3] += value * 0.488603 *  dir.x;
			SHCoeffs[4] += value * 1.092548 *  dir.x*dir.y;
			SHCoeffs[5] += value * 1.092548 *  dir.y*dir.z;
			SHCoeffs[6] += value * 0.315392 * (3.f*dir.z*dir.z -1.f);
			SHCoeffs[7] += value * 1.092548 *  dir.x * dir.z;
			SHCoeffs[8] += value * 0.546274 * (dir.x*dir.x - dir.y*dir.y);
		}

		SHCoeffs0 = vec4(SHCoeffs[0],SHCoeffs[7].x);
		SHCoeffs1 = vec4(SHCoeffs[1],SHCoeffs[7].y);
		SHCoeffs2 = vec4(SHCoeffs[2],SHCoeffs[7].z);
		SHCoeffs3 = vec4(SHCoeffs[3],SHCoeffs[8].x);
		SHCoeffs4 = vec4(SHCoeffs[4],SHCoeffs[8].y);
		SHCoeffs5 = vec4(SHCoeffs[5],SHCoeffs[8].z);
		SHCoeffs6 = vec4(SHCoeffs[6],1);
	}
#endif


#ifdef RENDERER
	uniform vec3			SHCoeffs[9];
	//uniform samplerCube		CubeTex;
	uniform sampler2D		PositionTex;
	uniform sampler2D		NormalTex;

	out vec4 				FragColor;
	const float 			c1 = 0.429043, 
							c2 = 0.511664, 
							c3 = 0.743125, 
							c4 = 0.886227, 
							c5 = 0.247708;

//	//--------------------------------------------------------------------------
//	float ClosestPower2(int _x, out int _exp)
//	{
//		float temp	= log(float(_x))/log(2.f);
//		_exp		= int(temp+0.5);
//		return pow(2.f,_exp);
//	}
//	//--------------------------------------------------------------------------
//	float Exponent2Layer( float _exponent, int _maxExponent, int _nTextures )
//	{
//		if(_exponent<=1.f) 			return 0.f;
//		if(_exponent>=_maxExponent) return 1.f;

//		int index;
//		float value		= ClosestPower2(int(_exponent), index);
//		float offset	= _exponent-value;
//		int inf			= offset>=0?index:index-1;
//		int sup			= offset>=0?index+1:index;

//		float eInf		= pow(2.f,inf);
//		float eSup		= pow(2.f,sup);
//		offset 			= (_exponent-eInf) / (eSup-eInf);

//		return (inf + offset) / float(_nTextures-1);
//	}
	//--------------------------------------------------------------------------
	// Convolve with a clamped cos
	// From Siggraph 02 An efficient representation for irradiance environment maps 
	// [Ravi Ramamorthi, Pat Hanrahan]
	// Compute the value of the kernel (including the scaling factor of the convolution)
	void main()
	{
		vec2 pix		= gl_FragCoord.xy / vec2(textureSize(PositionTex,0));
		vec3 n			= normalize(texture(NormalTex,pix).xyz);
		vec3 dRadiance	= 	c1 *  SHCoeffs[8] * (n.x*n.x - n.y*n.y) 
						+	c3 *  SHCoeffs[6] * n.z*n.z
						+	c4 *  SHCoeffs[0]
						-	c5 *  SHCoeffs[6] 
						+ 2*c1 * (SHCoeffs[4]*n.x*n.y + SHCoeffs[7]*n.x*n.z + SHCoeffs[5]*n.y*n.z)
						+ 2*c2 * (SHCoeffs[3]*n.x + SHCoeffs[1]*n.y + SHCoeffs[2]*n.z );

		vec3 sRadiance	= vec3(0);
		//vec3 sRadiance	= texture(CubeTex,n).xyz;
		FragColor	  	= vec4(dRadiance * INV_PI + sRadiance,1);
	}
#endif
