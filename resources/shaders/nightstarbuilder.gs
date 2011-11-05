//-----------------------------------------------------------------------------
#version 330 core

uniform mat4 Transformations[6];

in  float vIntensity[1];
out float gIntensity;

layout(points) in;
layout(points, max_vertices = 6) out;

void main()
{
	gIntensity	= vIntensity[0];
	for(int layer=0;layer<6;++layer)
	{
		gl_Layer = layer;
		gl_Position = Transformations[layer] * gl_in[0].gl_Position;
		EmitVertex();
		EndPrimitive();
	}
}
