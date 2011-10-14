//-----------------------------------------------------------------------------
#version 330

uniform mat4 Transform;

layout(location = 0) in  vec3 Position;

void main()
{
	gl_Position  = Transform * vec4(Position,1.f);
}

