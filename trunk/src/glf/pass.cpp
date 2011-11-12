//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/pass.hpp>
#include <glf/geometry.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	RenderSurface::RenderSurface(	unsigned int _width, 
									unsigned int _height):
	program("Surface")
	{
		ProgramOptions options = ProgramOptions::CreateVSOptions();
		options.AddResolution("SCREEN",_width,_height);
		program.Compile(options.Append(LoadFile(directory::ShaderDirectory + "surface.vs")),
						options.Append(LoadFile(directory::ShaderDirectory + "surface.fs")));

		textureUnit		= program["Texture"].unit;
		levelVar		= program["Level"].location;
		frameSize		= glm::vec2(_width,_height);
		glProgramUniform1i(program.id, 		  program["Texture"].location,		textureUnit);

		CreateScreenTriangle(vbo);
		vao.Add(vbo,semantic::Position,2,GL_FLOAT);

		glf::CheckError("Surface::Surface");
	}
	//--------------------------------------------------------------------------
	void RenderSurface::Draw(		const Texture2D& _texture,
									int _level)
	{
		assert(_texture.size.x==frameSize.x);
		assert(_texture.size.y==frameSize.y);

		glUseProgram(program.id);
		_texture.Bind(textureUnit);
		glProgramUniform1f(program.id, levelVar, float(_level));
		vao.Draw(GL_TRIANGLES,3,0);
		glf::CheckError("Surface::Draw");
	}
	//--------------------------------------------------------------------------
	RenderTarget::RenderTarget(				unsigned int _width, 
											unsigned int _height)
	{
		CreateScreenTriangle(vbo);
		vao.Add(vbo,semantic::Position,2,GL_FLOAT);
		texture.Allocate(GL_RGBA32F,_width,_height,true);
		texture.SetFiltering(GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR);
		texture.SetWrapping(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);

		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, texture.target, texture.id, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glf::CheckFramebuffer(framebuffer);
		glf::CheckError("RenderTarget::RenderTarget");
	}
	//--------------------------------------------------------------------------
	void RenderTarget::Bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
	}
	//--------------------------------------------------------------------------
	void RenderTarget::Unbind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER,0);
	}
	//--------------------------------------------------------------------------
	RenderTarget::~RenderTarget()
	{
		glDeleteFramebuffers(1, &framebuffer);
	}
	//--------------------------------------------------------------------------
	void RenderTarget::Draw() const
	{
		vao.Draw(GL_TRIANGLES,3,0);
	}
	//--------------------------------------------------------------------------
	void RenderTarget::AttachDepthStencil(const Texture2D& _depthStencilTex)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glFramebufferTexture2D(	GL_FRAMEBUFFER,
								GL_DEPTH_STENCIL_ATTACHMENT, 
								_depthStencilTex.target, 
								_depthStencilTex.id, 0);
		glf::CheckFramebuffer(framebuffer);
		glf::CheckError("AccumulationBuffer::AttachStencil");
	}
	//--------------------------------------------------------------------------
}
