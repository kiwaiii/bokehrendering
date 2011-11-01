//-----------------------------------------------------------------------------
#version 330 core

layout(location = ATTR_POSITION) in  vec3 	 Position;

void main()
{
	gl_Position =  vec4(Position,1.f);
}

