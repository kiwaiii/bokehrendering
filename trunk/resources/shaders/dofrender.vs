//-----------------------------------------------------------------------------
#version 410

uniform vec2		PixelScale;
uniform sampler2D	BokehPositionTex; //(x,y,scale)
uniform mat4	 	Transformation;
in vec3  			Position;
out vec2 			TexCoord;

void main()
{
	TexCoord	 = Position.xy;
	vec3 pos	 = texelFetch(BokehPositionTex,ivec2(gl_InstanceID,0),0).xyz;
	gl_Position  = Transformation * ( vec4(pos.z*Position.xy*PixelScale,0.f,1.f) + vec4(pos.xy * PixelScale,0,0) );
}

