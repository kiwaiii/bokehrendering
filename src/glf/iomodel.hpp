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
		void LoadScene(		const std::string& _folder,
							const std::string& _filename,
							const glm::mat4& _transform,
							ResourceManager& _resourceManager,
							SceneManager& _scene,
							bool _verbose=false);
	}
}

#endif
