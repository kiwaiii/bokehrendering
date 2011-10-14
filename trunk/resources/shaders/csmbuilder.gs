//------------------------------------------------------------------------------
#version 330 core
//------------------------------------------------------------------------------
uniform int  nCascades;
uniform mat4 Projections[4];
//------------------------------------------------------------------------------
layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;
//------------------------------------------------------------------------------
void main()
{
	for(int layer=0;layer<nCascades;++layer)
	{
		gl_Layer = layer;
		for(int i=0; i<3;++i)
		{
			gl_Position = Projections[layer] * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}
