//-----------------------------------------------------------------------------
//#version 330 core
#version 410 core

// Declare all the semantics
#define ATTR_POSITION	0

uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Model;

layout(location = ATTR_POSITION) 	in  vec3 Position;

void main()
{
	gl_Position  = Projection * View * Model * vec4(Position,1.f);
}

