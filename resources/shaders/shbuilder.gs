//-----------------------------------------------------------------------------
#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 27) out;

in  vec2 vTexCoord;
out vec3 gPosition;
out vec2 gTexCoord;

void main()
{
	for(int layer=0;layer<9;++layer)
	{
		gl_Layer = layer;
		for(int i=0; i<3;++i)
		{
			gl_Position = gl_in[i].gl_Position;
			gPosition	= gl_in[i].gl_Position.xyz;
			gTexCoord   = vTexCoord;
			EmitVertex();
		}
		EndPrimitive();
	}
}
