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

layout(location = ATTR_POSITION) 	in  vec3 Position;
layout(location = ATTR_NORMAL) 		in  vec3 Normal;
layout(location = ATTR_TEXCOORD) 	in  vec2 TexCoord;
layout(location = ATTR_TANGENT) 	in  vec3 Tangent;

out vec3 gPosition;
out vec3 gNormal;
out vec3 gTangent;
out vec2 gTexCoord;

void main()
{
	// Do not support non uniform scale
	mat3 model3x3= mat3(Model);
	gl_Position  = Transform * Model * vec4(Position,1.f);
	gPosition	 = model3x3 * Position;
	gNormal	 	 = model3x3 * Normal;
	gTangent 	 = model3x3 * Tangent;
	gTexCoord 	 = TexCoord;
}

