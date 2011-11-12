#version 420 core

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
uniform sampler2DArray  CubeMap;
uniform mat4            Transformations[6];
//------------------------------------------------------------------------------
layout(location = 0, index = 0) out vec4 SHCoeffs0;
layout(location = 1, index = 0) out vec4 SHCoeffs1;
layout(location = 2, index = 0) out vec4 SHCoeffs2;
layout(location = 3, index = 0) out vec4 SHCoeffs3;
layout(location = 4, index = 0) out vec4 SHCoeffs4;
layout(location = 5, index = 0) out vec4 SHCoeffs5;
layout(location = 6, index = 0) out vec4 SHCoeffs6;


//------------------------------------------------------------------------------
void main()
{
    // Compute solid angle weight
	// Each face is a 2x2 unit quad placed at 1 unit from the center
	// Thus the area of a face is 4
	// The solid angle is :
	// \Omega = cos\theta / r2 * texelArea
	// cos\theta is equal to 1/sqrt(r2)	
	ivec2 texSize	= textureSize(CubeMap,0).xy;
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
		vec3 value   = weight * textureLod(CubeMap,vec3(texCoord.x,texCoord.y,l),0).xyz;
		vec3 dir	 = (Transformations[l] * vec4(refDir,0)).xyz;

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

