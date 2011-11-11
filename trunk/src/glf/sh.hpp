#ifndef GLF_SH_HPP
#define GLF_SH_HPP

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glm/glm.hpp>
#include <glf/texture.hpp>
#include <glf/wrapper.hpp>
#include <glf/buffer.hpp>
#include <glf/pass.hpp>
#include <glf/gbuffer.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	class SHLight
	{	
	private:
					SHLight(			const SHLight&);
		SHLight		operator=(			const SHLight&);
	public:	
										SHLight();
		glm::vec3						coeffs[9];
	};
	//--------------------------------------------------------------------------
	class SHBuilder
	{
	private:
					SHBuilder(			const SHBuilder&);
		SHBuilder	operator=(			const SHBuilder&);
	public:
					SHBuilder(			int _resolution);
		void 		Project(			const TextureArray2D& 	_source, 
										SHLight& 				_coeffs,
										int 					_level=0);
	private:

		GLuint							texProjectionUnit;
		GLuint							shFrameProjection;
		TextureArray2D					shTexture;
		Program							programProjection;
		VertexBuffer3F					vbo;
		VertexArray						vao;
		int 							resolution;
	};
	//--------------------------------------------------------------------------
	class SHRenderer
	{
	private:
 					SHRenderer(			const SHRenderer&);
 		SHRenderer	operator=(			const SHRenderer&);
	public:
					SHRenderer(			int _w, 
										int _h);
		void 		Draw(				const SHLight&	_light,
										const GBuffer&	_gbuffer,
										const RenderTarget& _renderTarget);

		GLint 							diffuseTexUnit;
		GLint 							normalTexUnit;
		GLint							shLightVar;
		Program 						program;
	};
	//--------------------------------------------------------------------------
}
#endif

