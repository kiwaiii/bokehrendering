//-----------------------------------------------------------------------------
//#version 330 core
#version 410 core

uniform sampler2D   DiffuseTex;
uniform sampler2D   NormalTex;
uniform float       Roughness;

in  vec3 gPosition;
in  vec3 gNormal;
in  vec3 gTangent;
in  vec2 gTexCoord;

layout(location = 0, index = 0) out vec4 FragPosition;
layout(location = 1, index = 0) out vec3 FragNormal;
layout(location = 2, index = 0) out vec4 FragDiffuse;

void main()
{
	// Build tangent space
	vec3 gNNormal   = normalize(gNormal);
	vec3 gNTangent  = normalize(gTangent);
	vec3 gNBitangent= normalize(cross(gNNormal,gNTangent));

	// Extract normal and project it in world space
	vec3 normal  	= texture(NormalTex,gTexCoord).xyz*2.f - 1.f;
	FragPosition 	= vec4(gPosition,1);
	FragNormal   	= normalize(normal.x*gNTangent + normal.y*gNBitangent + normal.z*gNNormal);
	FragDiffuse  	= vec4(texture(DiffuseTex,gTexCoord).xyz,Roughness);
}
