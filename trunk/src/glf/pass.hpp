#ifndef GLF_PASS_HPP
#define GLF_PASS_HPP

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/utils.hpp>
#include <glf/wrapper.hpp>
#include <glf/scene.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	struct GBuffer
	{
	private:
					GBuffer(			const GBuffer&);
		GBuffer&	operator=(			const GBuffer&);
	public:
					GBuffer(			unsigned int _width, 
										unsigned int _height);
				   ~GBuffer(			);
		void 		Draw(				const glm::mat4& _projection,
										const glm::mat4& _view,
										const SceneManager& _scene);

		Program 						program;
		Texture2D 						positionTex;	// Position buffer (could be reconstruct from depth)
		Texture2D  						normalTex;		// World space normal buffer
		Texture2D 						diffuseTex;		// RGB : albedo / A : roughness
		Texture2D  						depthTex; 		// Depth/Stencil buffer

		GLint 	 						normalTexUnit;
		GLint 	 						diffuseTexUnit;
		GLint	 						roughnessVar;

		GLint	 						transformVar;
		GLint	 						modelVar;

		GLuint	 						framebuffer;
	};
	//--------------------------------------------------------------------------
	struct Surface
	{
	private:
					Surface(			const Surface&);
		Surface&	operator=(			const Surface&);
	public:
		GLint 							textureUnit;
		GLint 							levelVar;
		glm::vec2						frameSize;
		Program 						program;
		VertexBuffer3F					vbo;
		VertexArray						vao;

					Surface(			unsigned int _width, 
										unsigned int _height);
		void 		Draw(				const Texture2D& _texture,
										int 			 _level=0);
	};
	//--------------------------------------------------------------------------
	struct RenderTarget
	{
	private:
					RenderTarget(		const RenderTarget&);
		RenderTarget& operator=(		const RenderTarget&);
	public:
		glm::vec2						frameSize;
		Texture2D						texture;
		GLuint							framebuffer;
		glm::mat4						transformation;
		VertexBuffer3F					vbo;
		VertexArray						vao;

					RenderTarget(		unsigned int _width, 
										unsigned int _height);
				   ~RenderTarget(		);
		void		Draw(				) const;
		void		AttachDepthStencil(	const Texture2D& _depthStencilTex);
	};
	//--------------------------------------------------------------------------
/*
	struct AccumulationBuffer
	{
		//----------------------------------------------------------------------
		typedef SmartPointer<AccumulationBuffer> Ptr;
		//----------------------------------------------------------------------
		glm::vec2						frameSize;
		Texture2D						texture;
		GLuint							framebuffer;
		glm::mat4						transformation;
		VertexBuffer<glm::vec3>::Buffer vbuffer;
		//----------------------------------------------------------------------
	private:
					AccumulationBuffer(	unsigned int _width, 
										unsigned int _height);
	public:
				   ~AccumulationBuffer(	);
		static Ptr 	Create(				unsigned int _width, 
										unsigned int _height);
		void		AttachDepthStencil(	const Texture2D& _depthStencilTex);
//		void 		AddMode(			);
//		void 		MultiplyMode(		);
		//----------------------------------------------------------------------
	};

	// Min-Max
	struct MinMax
	{
		//----------------------------------------------------------------------
		typedef SmartPointer<MinMax> Ptr;
		//----------------------------------------------------------------------
	public:
		static Ptr  Create(				unsigned int _width, 
										unsigned int _height);
		void 		Get(				const Texture2D& _texture,
										float& _min, 
										float& _max);
				   ~MinMax(	);
	private:
					MinMax(				unsigned int _width, 
										unsigned int _height);

		Texture2D							texture;
		Program 						program;
		GLint							textureVar;
		GLint							textureUnit;
		GLint							transfoVar;
		GLint							lodVar;
		GLint							initPassVar;
		GLint							vbufferVar;
		int								nMipmaps;
		std::vector<glm::ivec2>			scissorRes;
		std::vector<glm::ivec2>			viewportRes;
		std::vector<GLuint>				framebuffers;
		VertexBuffer<glm::vec4>::Buffer vbuffer;
	};
*/
}
#endif

