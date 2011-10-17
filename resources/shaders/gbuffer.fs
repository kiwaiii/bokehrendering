//-----------------------------------------------------------------------------
//#version 330 core
#version 410 core

uniform sampler2D   DiffuseTex;
uniform sampler2D   NormalTex;
uniform float       Roughness;
uniform float       Specularity;

in  vec3  vPosition;
in  vec3  vNormal;
in  vec3  vTangent;
in  vec2  vTexCoord;
in  float vTBNsign;

layout(location = 0, index = 0) out vec4 FragPosition;
layout(location = 1, index = 0) out vec4 FragNormal;
layout(location = 2, index = 0) out vec4 FragDiffuse;

void main()
{
	// Build tangent space
	vec3 vNNormal   = normalize(vNormal);
	vec3 vNTangent  = normalize(vTangent);
	vec3 vNBitangent= normalize(cross(vNNormal,vNTangent)) * sign(vTBNsign);

	// Extract normal and project it in world space
	vec3 normal  	= texture(NormalTex,vTexCoord).xyz*2.f - 1.f;
	FragPosition 	= vec4(vPosition,1);
	FragNormal   	= vec4(normalize(normal.x*vNTangent + normal.y*vNBitangent + normal.z*vNNormal),Specularity);
	FragDiffuse  	= vec4(texture(DiffuseTex,vTexCoord).xyz,Roughness);
}

