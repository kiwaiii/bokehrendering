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
	GBuffer::GBuffer(				unsigned int _width, 
									unsigned int _height):
	program("GBuffer"),
	diffuseTexUnit(INVALID_ID),
	normalTexUnit(INVALID_ID),
	roughnessVar(INVALID_ID),
	specularityVar(INVALID_ID),
	transformVar(INVALID_ID),
	modelVar(INVALID_ID),
	framebuffer(INVALID_ID)
	{
		program.Compile(ProgramOptions::CreateVSOptions().Append(LoadFile(directory::ShaderDirectory + "gbuffer.vs")),
						LoadFile(directory::ShaderDirectory + "gbuffer.fs"));

		transformVar	= program["Transform"].location;
		modelVar		= program["Model"].location;

		diffuseTexUnit	= program["DiffuseTex"].unit;
		normalTexUnit	= program["NormalTex"].unit;
		roughnessVar	= program["Roughness"].location;
		specularityVar	= program["Specularity"].location;

		// Initialize G-Buffer textures
		positionTex.Allocate(GL_RGBA32F,_width,_height);
		normalTex.Allocate(GL_RGBA16F,_width,_height);
		diffuseTex.Allocate(GL_RGBA16F,_width,_height);
		depthTex.Allocate(GL_DEPTH32F_STENCIL8,_width,_height);
		positionTex.SetWrapping(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
		normalTex.SetWrapping(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
		diffuseTex.SetWrapping(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
		depthTex.SetWrapping(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);

		// Initialize framebuffer
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);

		// Attach output textures
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0 + program.Output("FragPosition"), positionTex.target, positionTex.id, 0);
		glf::CheckFramebuffer(framebuffer);
		glf::Info("FragPosition out : %d",program.Output("FragPosition"));

		glBindTexture(diffuseTex.target,diffuseTex.id);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0 + program.Output("FragDiffuse"), diffuseTex.target, diffuseTex.id, 0);
		glf::CheckFramebuffer(framebuffer);
		glf::Info("FragDiffuse out : %d",program.Output("FragDiffuse"));
		
		glBindTexture(normalTex.target,normalTex.id);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0 + program.Output("FragNormal"), normalTex.target, normalTex.id, 0);
		glf::CheckFramebuffer(framebuffer);
		glf::Info("FragNormal out : %d",program.Output("FragNormal"));

		glBindTexture(depthTex.target,depthTex.id);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT, depthTex.target, depthTex.id, 0);
		glf::CheckFramebuffer(framebuffer);

		GLenum drawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
		glDrawBuffers(3,drawBuffers);

		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Bind input textures
		glProgramUniform1i(program.id, program["DiffuseTex"].location,  diffuseTexUnit);
		glProgramUniform1i(program.id, program["NormalTex"].location,   normalTexUnit);

		glf::CheckError("GBuffer::GBuffer");
	}
	//--------------------------------------------------------------------------
	GBuffer::~GBuffer()
	{
		glDeleteFramebuffers(1,&framebuffer);
	}	
	//--------------------------------------------------------------------------
	void GBuffer::Draw(				const glm::mat4& _projection,
									const glm::mat4& _view,
									const SceneManager& _scene)
	{
		glUseProgram(program.id);
		glm::mat4 transform = _projection * _view;
		glProgramUniformMatrix4fv(program.id, transformVar,  1, GL_FALSE, &transform[0][0]);

		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Render at the same resolution than the original window
		// Draw all objects
		int nMeshes = int(_scene.regularMeshes.size());
		for(int i=0;i<nMeshes;++i)
		{
			glProgramUniformMatrix4fv(program.id, 	modelVar,  1, 	GL_FALSE, &_scene.transformations[i][0][0]);
			glProgramUniform1f(program.id,			roughnessVar,	_scene.regularMeshes[i].roughness);
			glProgramUniform1f(program.id,			specularityVar,	_scene.regularMeshes[i].specularity);

			_scene.regularMeshes[i].diffuseTex->Bind(diffuseTexUnit);
			_scene.regularMeshes[i].normalTex->Bind(normalTexUnit);
			_scene.regularMeshes[i].Draw();
		}

		glBindFramebuffer(GL_FRAMEBUFFER,0);
	}
	//--------------------------------------------------------------------------
	RenderSurface::RenderSurface(	unsigned int _width, 
									unsigned int _height):
	program("Surface")
	{
		program.Compile(LoadFile(directory::ShaderDirectory + "surface.vs"),
						LoadFile(directory::ShaderDirectory + "surface.fs"));

		textureUnit		= program["Texture"].unit;
		levelVar		= program["Level"].location;

		frameSize		= glm::vec2(_width,_height);
		glm::mat4 proj	= glm::ortho(-0.5f,0.5f,-0.5f,0.5f,0.1f,100.f);
		glm::mat4 view 	= glm::lookAt(glm::vec3(0.5f,0.5f,5.0f),glm::vec3(0.5f,0.5f,-1.0f),glm::vec3(0.0f,1.0f,0.0f));
		glm::mat4 trans = proj*view;

		glProgramUniformMatrix4fv(program.id, program["Transform"].location,	1, GL_FALSE, &trans[0][0]);
		glProgramUniform1i(program.id, 		  program["Texture"].location,		textureUnit);
		glProgramUniform2f(program.id, 		  program["FrameSize"].location,	frameSize.x,frameSize.y);

		CreateQuad(vbo);
		vao.Add(vbo,semantic::Position,3,GL_FLOAT);

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
		vao.Draw(GL_TRIANGLES,6,0);
		glf::CheckError("Surface::Draw");
	}
	//--------------------------------------------------------------------------
	RenderTarget::RenderTarget(				unsigned int _width, 
											unsigned int _height)
	{
		CreateQuad(vbo);
		vao.Add(vbo,semantic::Position,3,GL_FLOAT);
		transformation	= ScreenQuadTransform();
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
		vao.Draw(GL_TRIANGLES,6,0);
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
