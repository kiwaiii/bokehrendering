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
	transformVar(INVALID_ID),
	modelVar(INVALID_ID),
	framebuffer(INVALID_ID)
	{
		program.Compile(LoadFile("../resources/shaders/gbuffer.vs"),
						LoadFile("../resources/shaders/gbuffer.fs"));

		transformVar	= program["Transform"].location;
		modelVar		= program["Model"].location;

		diffuseTexUnit	= program["DiffuseTex"].unit;
		normalTexUnit	= program["NormalTex"].unit;
		roughnessVar	= program["Roughness"].location;
		specularityVar	= program["Specularity"].location;

		// Initialize G-Buffer textures
		positionTex.Allocate(GL_RGBA32F,_width,_height);
		normalTex.Allocate(GL_RGBA32F,_width,_height);
		diffuseTex.Allocate(GL_RGBA32F,_width,_height);
		depthTex.Allocate(GL_DEPTH32F_STENCIL8,_width,_height);

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
		int nMeshes = _scene.regularMeshes.size();
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
		program.Compile(LoadFile("../resources/shaders/surface.vs"),
						LoadFile("../resources/shaders/surface.fs"));

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
//	//--------------------------------------------------------------------------
//	void AccumulationBuffer::AddMode()
//	{
//		glEnable(GL_BLEND);
//		glBlendFunc( GL_ONE, GL_ONE);
//		glBlendEquation(GL_FUNC_ADD);
//	}
//	//--------------------------------------------------------------------------
//	void AccumulationBuffer::MultiplyMode()
//	{
////		glEnable(GL_BLEND);
////		glBlendFunc( GL_ZERO, GL_SRC_ALPHA);
////		glBlendEquation(GL_FUNC_ADD);
//	}
/*
	//--------------------------------------------------------------------------
	AccumulationBuffer::Ptr AccumulationBuffer::Create(	unsigned int _width, 
														unsigned int _height)
	{
		return AccumulationBuffer::Ptr(new AccumulationBuffer(_width,_height));
	}
	//--------------------------------------------------------------------------
	AccumulationBuffer::AccumulationBuffer(				unsigned int _width, 
														unsigned int _height)
	{
		glm::mat4 proj	= glm::ortho(-0.5f,0.5f,-0.5f,0.5f,0.1f,100.f);
		glm::mat4 view 	= glm::lookAt(glm::vec3(0.5f,0.5f,5.0f),glm::vec3(0.5f,0.5f,-1.0f),glm::vec3(0.0f,1.0f,0.0f));
		transformation	= proj*view;
		CreateQuad(vbuffer);

		texture.Allocate(GL_RGBA32F,_width,_height);

		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, texture.target, texture.id, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glf::CheckFramebuffer(framebuffer);
		glf::CheckError("AccumulationBuffer::AccumulationBuffer");
	}
	//--------------------------------------------------------------------------
	AccumulationBuffer::~AccumulationBuffer()
	{
		glDeleteFramebuffers(1, &framebuffer);
	}
	//--------------------------------------------------------------------------
	void AccumulationBuffer::AttachDepthStencil(const Texture2D& _depthStencilTex)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT, _depthStencilTex.target, _depthStencilTex.id, 0);
		glf::CheckFramebuffer(framebuffer);
		glf::CheckError("AccumulationBuffer::AttachStencil");
	}
//	//--------------------------------------------------------------------------
//	void AccumulationBuffer::AddMode()
//	{
//		glEnable(GL_BLEND);
//		glBlendFunc( GL_ONE, GL_ONE);
//		glBlendEquation(GL_FUNC_ADD);
//	}
//	//--------------------------------------------------------------------------
//	void AccumulationBuffer::MultiplyMode()
//	{
////		glEnable(GL_BLEND);
////		glBlendFunc( GL_ZERO, GL_SRC_ALPHA);
////		glBlendEquation(GL_FUNC_ADD);
//	}


	//--------------------------------------------------------------------------
	MinMax::Ptr  MinMax::Create(	unsigned int _width, 
									unsigned int _height)
	{
		return MinMax::Ptr( new MinMax(_width,_height));
	}
	//--------------------------------------------------------------------------
	void MinMax::Get(	const Texture2D& _texture,
						float& _min, 
						float& _max)
	{
		glDisable(GL_DEPTH_TEST);
			
		glUseProgram(program.id);
		glf::CheckError("Check program");
		for(int i=0;i<nMipmaps;++i)
		{
			glBindFramebuffer(GL_FRAMEBUFFER,framebuffers[i]);
			glViewport(0,0,viewportRes[i].x,viewportRes[i].y);

			if(i==0)
			{
				_texture.Bind(textureUnit);
				glProgramUniform1i(program.id, lodVar,0);
				glProgramUniform1i(program.id, initPassVar,1);
			}
			else
			{
				texture.Bind(textureUnit);
				glProgramUniform1i(program.id, lodVar,i-1);
				glProgramUniform1i(program.id, initPassVar,0);
			}

			glBindBuffer(GL_ARRAY_BUFFER, vbuffer.id);
			glVertexAttribPointer(	vbufferVar, 
									4, 
									GL_FLOAT, 
									false, 
									sizeof(glm::vec4),
									GLF_BUFFER_OFFSET(0));
			glEnableVertexAttribArray(vbufferVar);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}		
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glEnable(GL_DEPTH_TEST);

		float values[2];
		glBindTexture(texture.target,texture.id);
		glGetTexImage(texture.target,nMipmaps-1,GL_RG,GL_FLOAT,(unsigned char*)values);
		_min = values[0];
		_max = values[1];
	}
	//--------------------------------------------------------------------------
	MinMax::~MinMax(	)
	{
	
	}
	//--------------------------------------------------------------------------
	MinMax::MinMax(		unsigned int _width, 
						unsigned int _height):
	program("MinMax")
	{
		int side		= std::max(_width,_height);
		int pow2		= NearestSuperiorPowerOf2(side)>>1;
		nMipmaps		= MipmapLevels(pow2);

		texture.Allocate(GL_RG32F,pow2,pow2,true);
		texture.SetFiltering(GL_LINEAR,GL_LINEAR);
		texture.SetWrapping(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
		glf::CheckError("MinMax");

		viewportRes.resize(nMipmaps);
		framebuffers.resize(nMipmaps);
		for(int i=0;i<nMipmaps;++i)
		{
			int current		= pow2>>i;
			viewportRes[i]	= glm::ivec2(current,current);

			glGenFramebuffers(1,&framebuffers[i]);
			glBindFramebuffer(GL_FRAMEBUFFER,framebuffers[i]);		
			glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, texture.target, texture.id, i);	
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glf::CheckError("MinMax");
			glf::CheckFramebuffer(framebuffers[i]);
			assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		}
		glBindFramebuffer(GL_FRAMEBUFFER,0);

		program.Compile(LoadFile("../resources/shaders/minmax.vs"),
						LoadFile("../resources/shaders/minmax.fs"));

		transfoVar 		= program["Transfo"].location;
		textureVar		= program["Texture"].location;
		textureUnit		= program["Texture"].unit;
		lodVar			= program["Lod"].location;
		vbufferVar		= program["Position"].location;
		initPassVar		= program["InitPass"].location;

		glm::mat4 proj	= glm::ortho(-0.5f,0.5f,-0.5f,0.5f,0.1f,100.f);
		glm::mat4 view	= glm::lookAt(glm::vec3(0.5f,0.5f,5.0f),glm::vec3(0.5f,0.5f,-1.0f),glm::vec3(0.0f,1.0f,0.0f));
		glm::mat4 transfo = proj * view;

		glProgramUniformMatrix4fv(program.id, transfoVar,  1, GL_FALSE, &transfo[0][0]);
		glProgramUniform1i(program.id, 		  textureVar,  textureUnit);

		CreateQuad(vbuffer);
	}
*/
}


#if 0
			//if(i==0)
			//{
			//	float values[32];
			//	glBindTexture(texture.target,texture.id);
			//	glGetTexImage(texture.target,0,GL_RG,GL_FLOAT,(unsigned char*)values);
			//	Info("%f %f   %f %f   %f %f   %f %f",values[0],values[1],values[2],values[3],values[4],values[5],values[6],values[7]);
			//	Info("%f %f   %f %f   %f %f   %f %f",values[8],values[9],values[10],values[11],values[12],values[13],values[14],values[15]);
			//	Info("%f %f   %f %f   %f %f   %f %f",values[16],values[17],values[18],values[19],values[20],values[21],values[22],values[23]);
			//	Info("%f %f   %f %f   %f %f   %f %f",values[24],values[25],values[26],values[27],values[28],values[29],values[30],values[31]);
			//}

			if(i==0)
			{
				float values[8];
				glBindTexture(texture.target,texture.id);
				glGetTexImage(texture.target,0,GL_RG,GL_FLOAT,(unsigned char*)values);
				Info("%f %f    %f %f",values[0],values[1],values[2],values[3]);
				Info("%f %f    %f %f",values[4],values[5],values[6],values[7]);
			}

			if(i==1)
			{
				float values[2];
				glBindTexture(texture.target,texture.id);
				glGetTexImage(texture.target,1,GL_RG,GL_FLOAT,(unsigned char*)values);
				Info("%f %f",values[0],values[1]);
			}

			glBindFramebuffer(GL_FRAMEBUFFER,0);

			{
				float values[9];
				glBindTexture(_texture.target,_texture.id);
				glGetTexImage(_texture.target,0,GL_RG,GL_FLOAT,(unsigned char*)values);
				Info("%f %f %f",values[0],values[1],values[2]);
				Info("%f %f %f",values[3],values[4],values[5]);
				Info("%f %f %f",values[6],values[7],values[8]);
			}
			if(i==0)
			{
				float values[32];
				glBindTexture(_texture.target,_texture.id);
				glGetTexImage(_texture.target,0,GL_RG,GL_FLOAT,(unsigned char*)values);
				Info("%f %f   %f %f   %f %f   %f %f",values[0],values[1],values[2],values[3],values[4],values[5],values[6],values[7]);
				Info("%f %f   %f %f   %f %f   %f %f",values[8],values[9],values[10],values[11],values[12],values[13],values[14],values[15]);
				Info("%f %f   %f %f   %f %f   %f %f",values[16],values[17],values[18],values[19],values[20],values[21],values[22],values[23]);
				Info("%f %f   %f %f   %f %f   %f %f",values[24],values[25],values[26],values[27],values[28],values[29],values[30],values[31]);
			}

			if(i==1)
			{
				float values[8];
				glBindTexture(texture.target,texture.id);
				glGetTexImage(texture.target,0,GL_RG,GL_FLOAT,(unsigned char*)values);
				Info("%f %f    %f %f",values[0],values[1],values[2],values[3]);
				Info("%f %f    %f %f",values[4],values[5],values[6],values[7]);
			}

			if(i==2)
			{
				float values[2];
				glBindTexture(texture.target,texture.id);
				glGetTexImage(texture.target,1,GL_RG,GL_FLOAT,(unsigned char*)values);
				Info("%f %f",values[0],values[1]);
			}
		

#endif
