//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/io/scene.hpp>
#include <glf/io/model.hpp>
#include <glf/io/config.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace glf
{
	namespace io
	{
		//----------------------------------------------------------------------
		void LoadScene(		const std::string& _filename,
							ResourceManager& _resourceManager,
							SceneManager& _scene,
							bool _verbose)
		{
			// Load configuration file
			glf::io::ConfigLoader loader;
			glf::io::ConfigNode* root	= loader.Load(_filename);

			// Load models
			glf::io::ConfigNode* geometriesNode = loader.GetNode(root,"geometries");
			int nGeometries = loader.GetCount(geometriesNode);
			for(int i=0;i<nGeometries;++i)
			{
				glf::io::ConfigNode* geometryNode = loader.GetNode(geometriesNode,i);

				std::string name			= loader.GetString(geometryNode,"name");
				std::string folder			= loader.GetString(geometryNode,"folder");
				std::string filename		= loader.GetString(geometryNode,"file");
				glm::vec3 translate			= loader.GetVec3(geometryNode,"translate");
				glm::vec3 rotate			= loader.GetVec3(geometryNode,"rotate");
				float scale					= loader.GetFloat(geometryNode,"scale");
				glm::mat4 transform			=	glm::translate(translate.x,translate.y,translate.z) *
												glm::rotate(rotate.z,0.f,0.f,1.f) *
												glm::rotate(rotate.y,0.f,1.f,0.f) *
												glm::rotate(rotate.x,1.f,0.f,0.f) *
												glm::scale(scale,scale,scale);

				LoadModel(	glf::directory::ModelDirectory + folder + "/",
							filename,
							transform,
							_resourceManager,
							_scene,
							_verbose);
			}

			// Load lights
			//TODO

			// Load camera
			//TODO
		}
		//----------------------------------------------------------------------
	}
}
