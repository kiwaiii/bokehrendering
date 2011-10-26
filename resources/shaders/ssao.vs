//-----------------------------------------------------------------------------
#version 410

#define ATTR_POSITION	0

uniform mat4 Transformation;
layout(location = ATTR_POSITION) in vec3 Position;

void main()
{
	gl_Position  = Transformation * vec4(Position,1.f);
}

