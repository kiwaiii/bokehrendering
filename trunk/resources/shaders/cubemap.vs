#version 420 core

uniform mat4 Transformation;
layout(location = ATTR_POSITION) in	vec3 Position;
layout(location = ATTR_TEXCOORD) in	vec3 TexCoord;

out	vec3 gTexCoord;
out	vec3 gPosition;

void main()
{
	gl_Position = Transformation * vec4(Position,1.f);
	gTexCoord	= TexCoord;
	gPosition	= Position;
}

