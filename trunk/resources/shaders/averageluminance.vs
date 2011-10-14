//-----------------------------------------------------------------------------
#version 330 core
uniform mat4 Transformation;
layout(location = 0) 	in  vec3 Position;
void main()
{
	gl_Position  = Transformation * vec4(Position,1.f);
}

