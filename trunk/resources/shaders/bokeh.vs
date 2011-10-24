//-----------------------------------------------------------------------------
#version 410

uniform vec2		PixelScale;
uniform sampler2D	BokehPositionTex; //(x,y,scale)
uniform sampler2D	BokehColorTex;

in vec3  			Position;
out float 			Radius;
out vec4 			Color;

void main()
{
	ivec2 bufSize, coord;
	bufSize 	 = textureSize(BokehPositionTex,0).xy;
	coord.y 	 = int(floor(gl_InstanceID/bufSize.y));
	coord.x 	 = gl_InstanceID - coord.y*bufSize.y;
	Color		 = texelFetch(BokehColorTex,coord,0);
	vec3 pos	 = texelFetch(BokehPositionTex,coord,0).xyz;
	Radius		 = pos.z;
	gl_Position	 = vec4( (Position.xy+pos.xy)*PixelScale,0,1);
}

