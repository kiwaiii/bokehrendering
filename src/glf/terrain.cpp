//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/terrain.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	TerrainMesh::TerrainMesh(	const glm::vec2 _terrainSize,
								const glm::vec2 _terrainOffset,
								Texture2D* _diffuseTexture,
								Texture2D* _normalTexture,
								Texture2D* _heightTexture,
								int _tileResolution):
	tileOffset(_terrainOffset),
	terrainSize(_terrainSize),
	diffuseTex(_diffuseTexture),
	normalTex(_normalTexture),
	heightTex(_heightTexture)
	{
		assert(glf::CheckError("TerrainMesh::TerrainMesh"));
		Tesselation(_tileResolution,1,16,20);
	}
	//--------------------------------------------------------------------------
	void TerrainMesh::Tesselation(	int   _tileResolution,
									float _depthFactor,
									float _tessFactor,
									float _projFactor)
	{
		// Set LOD parameters
		tileCount 		= glm::ivec2(diffuseTex->size.x/_tileResolution,diffuseTex->size.y/_tileResolution);
		tileSize  		= glm::vec2(terrainSize.x / float(tileCount.x),terrainSize.x / float(tileCount.y));
		depthFactor		= _depthFactor;
		tessFactor 		= _tessFactor;
		projFactor 		= _projFactor;
	}
	//--------------------------------------------------------------------------
	void TerrainMesh::Draw(		bool _drawWireframe) const
	{
		if(_drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// TODO : Add frustum culling 
		glPatchParameteri(GL_PATCH_VERTICES, 4);
		primitive->Draw(GL_PATCHES, 4, 0, tileCount.x*tileCount.y);

		if(_drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		assert(glf::CheckError("TerrainMesh::Draw"));
	}
	//--------------------------------------------------------------------------
	BBox TerrainMesh::Bound() const
	{
		BBox bound;
		bound.pMin = glm::vec3(tileOffset,0);
		bound.pMax = glm::vec3(tileOffset+terrainSize,fabs(depthFactor));
		return bound;
	}
}

