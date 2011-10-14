//-----------------------------------------------------------------------------
#version 410

uniform vec2		PixelScale;
uniform sampler2D	SampleTex; //(x,y,scale)
uniform sampler2D	ColorTex;

in vec3  			Position;
out float 			Radius;
out vec4 			Color;

void main()
{
	ivec2 bufSize, coord;
	bufSize 	 = textureSize(SampleTex,0).xy;
	coord.y 	 = int(floor(gl_InstanceID/bufSize.y));
	coord.x 	 = gl_InstanceID - coord.y*bufSize.y;
	Color		 = texelFetch(ColorTex,coord,0);
	vec3 pos	 = texelFetch(SampleTex,coord,0).xyz;
	Radius		 = pos.z;
	gl_Position	 = vec4( (Position.xy+pos.xy)*PixelScale,0,1);
}

