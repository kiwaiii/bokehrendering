#version 420 core

#ifdef GBUFFER
	uniform sampler2D	HeightTex;
	uniform mat4		Transform;
	uniform vec2		TileOffset;
	uniform vec2		TileSize;
	uniform ivec2		TileCount;

	layout(location = ATTR_POSITION) in vec2 Position;
	out ivec2 vTileCoord;
	out vec2  vTexCoord;
	out vec3  vProjPosition;

	void main()
	{
		ivec2 tileCoord;
		tileCoord.x			= gl_InstanceID % TileCount.x;
		tileCoord.y			= gl_InstanceID / TileCount.x;
		float height		= textureLod(HeightTex,vec2((Position.x+tileCoord.x)/float(TileCount.x),(Position.y+tileCoord.y)/float(TileCount.y)),0).x;
		vec4 worldPosition	= vec4(TileOffset.x + (Position.x+tileCoord.x)*TileSize.x,TileOffset.y + (Position.y+tileCoord.y)*TileSize.y,0,1);
		gl_Position			= worldPosition;
		vTileCoord			= tileCoord;
		vec4 tmp 			= Transform * vec4(worldPosition.xy,height,1);
		vProjPosition		= tmp.xyz / tmp.w;
	}
#endif

#ifdef CSM_BUILDER
	uniform sampler2D	HeightTex;
	uniform mat4		View;
	uniform mat4		Projections[MAX_CASCADES];
	uniform vec2		TileOffset;
	uniform vec2		TileSize;
	uniform ivec2		TileCount;

	layout(location = ATTR_POSITION) in vec2 Position;
	out ivec2 vTileCoord;
	out vec2  vTexCoord;
	out vec3  vProjPosition;

	void main()
	{
		ivec2 tileCoord;
		tileCoord.x			= gl_InstanceID % TileCount.x;
		tileCoord.y			= gl_InstanceID / TileCount.x;
		float height		= textureLod(HeightTex,vec2((Position.x+tileCoord.x)/float(TileCount.x),(Position.y+tileCoord.y)/float(TileCount.y)),0).x;
		vec4 worldPosition	= vec4(TileOffset.x + (Position.x+tileCoord.x)*TileSize.x,TileOffset.y + (Position.y+tileCoord.y)*TileSize.y,0,1);
		gl_Position			= worldPosition;
		vTileCoord			= tileCoord;
		vec4 tmp 			= Projections[0] * View * vec4(worldPosition.xy,height,1);
		vProjPosition		= tmp.xyz / tmp.w;
	}
#endif
