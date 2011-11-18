#version 420 core

#if (defined BUILDER || defined RENDERER)
	layout(location = ATTR_POSITION) in  vec2 Position;
	void main()
	{
		gl_Position = vec4(Position,1,1);
	}
#endif

