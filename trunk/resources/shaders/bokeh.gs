//-----------------------------------------------------------------------------
#version 330 core
//-----------------------------------------------------------------------------
uniform mat4	 	Transformation;
uniform vec2		PixelScale;
in  float 			Radius[1];
in  vec4 			Color[1];
out vec4			Radiance;
out vec2  			TexCoord;

layout(points) in;
layout(triangle_strip, max_vertices = 6) out;
//-----------------------------------------------------------------------------
void main()
{
	gl_Layer 	 = 0;
	vec4 offsetx = vec4(PixelScale.x*Radius[0],0,0,0);
	vec4 offsety = vec4(0,PixelScale.y*Radius[0],0,0);
	Radiance 	 = Color[0];


	// First triangle
	gl_Position = Transformation * ( gl_in[0].gl_Position - offsetx - offsety);
	TexCoord	= vec2(0,0);
	EmitVertex();

	gl_Position = Transformation * ( gl_in[0].gl_Position + offsetx - offsety);
	TexCoord	= vec2(1,0);
	EmitVertex();

	gl_Position = Transformation * ( gl_in[0].gl_Position + offsetx + offsety);
	TexCoord	= vec2(1,1);
	EmitVertex();
	EndPrimitive();


	// Second triangle
	gl_Position = Transformation * ( gl_in[0].gl_Position - offsetx - offsety);
	TexCoord	= vec2(0,0);
	EmitVertex();

	gl_Position = Transformation * ( gl_in[0].gl_Position + offsetx + offsety);
	TexCoord	= vec2(1,1);
	EmitVertex();

	gl_Position = Transformation * ( gl_in[0].gl_Position - offsetx + offsety);
	TexCoord	= vec2(0,1);
	EmitVertex();
	EndPrimitive();
}
