#version 330 core

uniform vec3	StarFactor;
in  float		gIntensity;
out vec4		FragColor;


void main()
{
	FragColor = vec4(gIntensity*StarFactor,1);
}

