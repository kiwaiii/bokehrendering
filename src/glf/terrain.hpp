#ifndef GLF_TERRAIN_HPP
#define GLF_TERRAIN_HPP

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/buffer.hpp>
#include <glf/bound.hpp>
#include <glf/wrapper.hpp>
#include <glf/texture.hpp>
#include <glm/glm.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	class TerrainMesh
	{
	public:
				TerrainMesh(				const glm::vec2 _terrainSize,
											const glm::vec2 _terrainOffset,
											Texture2D* _diffuseTexture,
											Texture2D* _normalTexture,
											Texture2D* _heightTexture,
											int _tileResolution=32);
		void 	Draw(						bool _drawWireframe=false) const;
		void	Tesselation(				int   _tileResolution,
											float _depthFactor,
											float _tessFactor,
											float _projFactor);
		BBox	Bound(						) const;
	public:
		glf::VertexArray*					primitive;

		glm::vec2							tileSize;		// Tile size
		glm::ivec2							tileCount;		// Number of tiles for the terrain
		glm::vec2							tileOffset; 	// World space offset
		glm::vec2							terrainSize;	// Terrain size (tileCount * tileSize)

		float								depthFactor;	// Depth of the heightfield
		float								tessFactor;		// Tesselation factor ]0,32]
		float								projFactor;		// For correcting projection estimation

		glf::Texture2D*						diffuseTex;
		glf::Texture2D*						normalTex;
		glf::Texture2D*						heightTex;
	};
}

#endif
