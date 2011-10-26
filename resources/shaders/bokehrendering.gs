//-----------------------------------------------------------------------------
#version 330 core
//-----------------------------------------------------------------------------
uniform mat4	 	Transformation;
uniform vec2		PixelScale;
in  float 			vRadius[1];
in  float 			vDepth[1];
in  vec4 			vColor[1];
out vec4			gColor;
out float			gDepth;
out vec2  			gTexCoord;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
//-----------------------------------------------------------------------------
void main()
{
	gl_Layer 	 = 0;
	gColor		 = vColor[0];
	gDepth		 = vDepth[0];
	vec4 offsetx = vec4(PixelScale.x*vRadius[0],0,0,0);
	vec4 offsety = vec4(0,PixelScale.y*vRadius[0],0,0);

	// Expand point into a quad
	gl_Position = Transformation * (gl_in[0].gl_Position - offsetx - offsety);
	gTexCoord	= vec2(0,0);
	EmitVertex();
	gl_Position = Transformation * (gl_in[0].gl_Position + offsetx - offsety);
	gTexCoord	= vec2(1,0);
	EmitVertex();
	gl_Position = Transformation * (gl_in[0].gl_Position - offsetx + offsety);
	gTexCoord	= vec2(0,1);
	EmitVertex();
	gl_Position = Transformation * (gl_in[0].gl_Position + offsetx + offsety);
	gTexCoord	= vec2(1,1);
	EmitVertex();

	EndPrimitive();
}
