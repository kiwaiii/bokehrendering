#ifndef GLF_IO_MODEL_HPP
#define GLF_IO_MODEL_HPP

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <string>
#include <glf/scene.hpp>
#include <glf/helper.hpp>

namespace glf
{
	namespace io
	{
		void LoadModel(		const std::string& _folder,
							const std::string& _filename,
							const glm::mat4& _transform,
							ResourceManager& _resourceManager,
							SceneManager& _scene,
							bool _verbose=false);

		void LoadTerrain(	const std::string& _folder,
							const std::string& _diffuseTex,
							const std::string& _normalTex,
							const std::string& _heightTex,
							const glm::vec2& _terrainSize,
							const glm::vec3& _terrainOffset,
							int _tileResolution,
							float _heightFactor,
							float _tessFactor,
							float _projFactor,
							ResourceManager& _resourceManager,
							SceneManager& _scene,
							bool _verbose=false);
	}
}

#endif
