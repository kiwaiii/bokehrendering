#ifndef GLF_POSTPROCESSOR_HPP
#define GLF_POSTPROCESSOR_HPP

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/buffer.hpp>
#include <glf/wrapper.hpp>
#include <glf/texture.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	class PostProcessor
	{
	private:
					PostProcessor(		const PostProcessor&);
		PostProcessor operator=(		const PostProcessor&);

	public:
					PostProcessor(		unsigned int _width, 
										unsigned int _height);
					~PostProcessor(		);
		void 		Apply(				const Texture2D& _texture,
										float _toneExposure);

	private:
		struct ToneMapping
		{
										ToneMapping();
			GLint 						luminanceTexUnit;
			GLint 						inputTexUnit;
			GLint 						exposureVar;
			GLint 						keyValueVar;
			GLuint 						framebuffer;
			Program						program;
		};

		ToneMapping						toneMapping;
		VertexBuffer3F					vbo;
		VertexArray						vao;
	};
	//--------------------------------------------------------------------------
}
#endif

