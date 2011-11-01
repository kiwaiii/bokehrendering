//-----------------------------------------------------------------------------
#version 420 core

uniform mat4 View;
uniform mat4 Model;
layout(location = ATTR_POSITION) in  vec3 Position;

void main()
{
	gl_Position  = View * Model * vec4(Position,1.f);
}

