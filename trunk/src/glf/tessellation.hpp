#ifndef GLF_TESSELATION_HPP
#define GLF_TESSELATION_HPP

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/buffer.hpp>
#include <glf/wrapper.hpp>
#include <glf/texture.hpp>
#include <glm/glm.hpp>
#include <glf/scenegraph.hpp>

namespace glf
{
	class TerrainGeometry
	{
	public:
		//---------------------------------------------------------------------
		typedef	glf::SmartPointer<TerrainGeometry> Ptr;
		static 	Ptr Create(					);
		void 	Draw(						const glm::mat4& _proj,
											const glm::mat4& _view,
											const glm::mat4& _model,
											bool _drawWireframe=false,
											bool _drawFixView=false);
	private:		
		//---------------------------------------------------------------------
		TerrainGeometry();
	public:		
		//---------------------------------------------------------------------
		glf::Program						 program;
		glf::VertexBuffer<glm::vec2>::Buffer vbuffer;
		GLint 								 vbufferVar;
		GLint 								 projVar;
		GLint 								 viewVar;
		GLint 								 view2Var;
		GLint 								 modelVar;
		GLint 								 tileSizeVar;
		GLint 								 tileCountVar;
		GLint								 normalTexUnit;
		GLint								 heightTexUnit;
		GLint 								 projFactorVar;
		GLint 								 tessFactorVar;
		GLint								 depthFactorVar;

		glm::vec2							 tileSize;
		glm::ivec2							 tileCount;
		glm::mat4							 fixViewMat;
		float								 depthFactor;
		float								 tessFactor;
		float								 projFactor;

		// TODO : add culling

//		glf::Texture2D						 diffuseTex;
//		glf::Texture2D						 specularTex;
		glf::Texture2D						 normalTex;
		glf::Texture2D						 heightTex;
	};


	class PhongGeometry
	{
	public:
		//---------------------------------------------------------------------
		typedef	glf::SmartPointer<PhongGeometry> Ptr;
		static 	Ptr Create(					);
		void 	Draw(						const glm::mat4& _proj,
											const glm::mat4& _view,
											const glm::mat4& _model,
											bool _drawWireframe=false);
	private:		
		//---------------------------------------------------------------------
		PhongGeometry();
	public:		
		//---------------------------------------------------------------------
		glf::Program						 program;
		glf::VertexBuffer<glm::vec3>::Buffer vbuffer;
		glf::VertexBuffer<glm::vec3>::Buffer nbuffer;
		GLint 								 vbufferVar;
		GLint 								 nbufferVar;

		GLint 								 projVar;
		GLint 								 viewVar;
		GLint 								 modelVar;
		GLint 								 curvFactorVar;
		GLint 								 tessFactorVar;

		float								 curvFactor;
		float								 tessFactor;
	};

	void ToPhongGeometry(	const std::vector<Object::Ptr>& _objects,
							std::vector<PhongGeometry::Ptr>& _phongs);
}


#endif
