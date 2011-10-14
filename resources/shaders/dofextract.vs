//-----------------------------------------------------------------------------
#version 410

uniform mat4	 	Transformation;
in vec3  			Position;
out vec2 			TexCoord;

void main()
{
	TexCoord	 = Position.xy;
	gl_Position  = Transformation * vec4(Position,1);
}

