//-----------------------------------------------------------------------------
//#version 330 core
#version 410 core

// Declare all the semantics
#define ATTR_POSITION	0
#define ATTR_NORMAL		1
#define ATTR_TEXCOORD	2
#define ATTR_TANGENT	3
#define ATTR_COLOR		4

uniform mat4 Transform;
uniform mat4 Model;

layout(location = ATTR_POSITION) in vec3 Position;
layout(location = ATTR_COLOR)    in vec3 Color;

out vec3 gColor;
void main()
{
	gl_Position = Transform * Model * vec4(Position,1.f);
	gColor		= Color;
}

