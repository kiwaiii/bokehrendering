//-----------------------------------------------------------------------------
#version 330 core

#define ATTR_POSITION	0
layout(location = ATTR_POSITION) in  vec3 	 Position;

void main()
{
	gl_Position =  vec4(Position,1.f);
}

