//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/dof.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>
#include <glf/window.hpp>
#include <glf/rng.hpp>

#include <gli/image.hpp>
#include <gli/io.hpp>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

namespace glf
{
	//-------------------------------------------------------------------------
	DOFProcessor::DOFProcessor(int _w, int _h):
	maxBokehCount(_w*_h),
	vbufferVar(0)
	{
		glm::mat4 proj 		= glm::ortho(-0.5f,0.5f,-0.5f,0.5f,0.1f,100.f);
		glm::mat4 view 		= glm::lookAt(glm::vec3(0.5f,0.5f,5.0f),glm::vec3(0.5f,0.5f,-1.0f),glm::vec3(0.0f,1.0f,0.0f));
		glm::mat4 transform	= proj * view;

		glm::vec3* vertices;
		quadbuffer.Resize(6);
		vertices = quadbuffer.Lock();
		vertices[0] = glm::vec3(0,0,0);
		vertices[1] = glm::vec3(1,0,0);
		vertices[2] = glm::vec3(1,1,0);
		vertices[3] = glm::vec3(0,0,0);
		vertices[4] = glm::vec3(1,1,0);
		vertices[5] = glm::vec3(0,1,0);
		quadbuffer.Unlock();

		pointbuffer.Resize(1);
		vertices = pointbuffer.Lock();
		vertices[0] = glm::vec3(0,0,0);
		pointbuffer.Unlock();

		// Create texture for counting bokeh
		sampleTex.Allocate(GL_RGBA32F,_w,_h);
		sampleTex.SetFiltering(GL_NEAREST,GL_NEAREST);
		colorTex.Allocate(GL_RGBA32F,_w,_h);
		colorTex.SetFiltering(GL_NEAREST,GL_NEAREST);

		// Setup the indirect buffer
		indirectBuffer.Resize(1);
		DrawArraysIndirectCommand* indirectCmd = indirectBuffer.Lock();
		indirectCmd[0].count 				= 1;
		indirectCmd[0].primCount 			= 6;
		indirectCmd[0].first 				= 0;
		indirectCmd[0].reservedMustBeZero 	= 0;
		indirectBuffer.Unlock();

		// Create the texture proxy
		glGenTextures(1, &countTexID);
		glBindTexture(GL_TEXTURE_BUFFER, countTexID);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, indirectBuffer.id);

		// Create framebuffer for composition
		composeTex.Allocate(GL_RGBA32F,_w,_h);
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, composeTex.target, composeTex.id, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glf::CheckFramebuffer(framebuffer);

		// CoC Pass
		cocPass.program.Compile(LoadFile("../resources/shaders/dof.vs"),
								LoadFile("../resources/shaders/dof.fs"));

		assert( vbufferVar	== cocPass.program["Position"].location);
		cocPass.nearStartVar		= cocPass.program["NearStart"].location;
		cocPass.nearEndVar			= cocPass.program["NearEnd"].location;
		cocPass.farStartVar			= cocPass.program["FarStart"].location;
		cocPass.farEndVar			= cocPass.program["FarEnd"].location;
		cocPass.maxRadiusVar		= cocPass.program["MaxRadius"].location;
		cocPass.nSamplesVar			= cocPass.program["nSamples"].location;
		cocPass.intThresholdVar		= cocPass.program["IntThreshold"].location;
		cocPass.cocThresholdVar		= cocPass.program["CoCThreshold"].location;
		cocPass.viewMatVar			= cocPass.program["ViewMat"].location;
		cocPass.areaFactorVar		= cocPass.program["AreaFactor"].location;

		cocPass.positionTexUnit		= cocPass.program["PositionTex"].unit;
		cocPass.rotationTexUnit		= cocPass.program["RotationTex"].unit;
		cocPass.inputTexUnit		= cocPass.program["InputTex"].unit;
		cocPass.sampleTexUnit		= cocPass.program["SampleTex"].unit;
		cocPass.colorTexUnit		= cocPass.program["ColorTex"].unit;
		cocPass.countTexUnit		= cocPass.program["CountTex"].unit;

		glProgramUniformMatrix4fv(cocPass.program.id, cocPass.program["Transformation"].location,	1, GL_FALSE, &transform[0][0]);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["PositionTex"].location,		cocPass.positionTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["RotationTex"].location,		cocPass.rotationTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["InputTex"].location,			cocPass.inputTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["SampleTex"].location,		cocPass.sampleTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["ColorTex"].location,			cocPass.colorTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["CountTex"].location,			cocPass.countTexUnit);

		// Create and fill rotation texture
		RNG rng;
		cocPass.rotationTex.Allocate(GL_RG32F,_w,_h);
		glm::vec2* rotations = new glm::vec2[_w * _h];
		for(int y=0;y<_h;++y)
		for(int x=0;x<_w;++x)
		{	
			float theta 		= 2.f * M_PI * rng.RandomFloat();
			rotations[x+y*_w] 	= glm::vec2(cos(theta),sin(theta));
		}
		cocPass.rotationTex.Fill(GL_RG,GL_FLOAT,(unsigned char*)rotations);
		delete[] rotations;


		// Bokeh Pass
		bokehPass.program.Compile(	LoadFile("../resources/shaders/bokeh.vs"),
									LoadFile("../resources/shaders/bokeh.gs"),
									LoadFile("../resources/shaders/bokeh.fs"));

		assert( vbufferVar	== bokehPass.program["Position"].location);
		bokehPass.sampleTexUnit	= bokehPass.program["SampleTex"].unit;
		bokehPass.bokehTexUnit	= bokehPass.program["BokehTex"].unit;
		bokehPass.colorTexUnit	= bokehPass.program["ColorTex"].unit;
		bokehPass.attenuationVar= bokehPass.program["Attenuation"].location;

		glProgramUniformMatrix4fv(bokehPass.program.id,	bokehPass.program["Transformation"].location,	1, GL_FALSE, &transform[0][0]);
		glProgramUniform2f(bokehPass.program.id, 		bokehPass.program["PixelScale"].location,		1.f/_w, 1.f/_h);
		glProgramUniform1i(bokehPass.program.id, 		bokehPass.program["SampleTex"].location,		bokehPass.sampleTexUnit);
		glProgramUniform1i(bokehPass.program.id, 		bokehPass.program["BokehTex"].location,			bokehPass.bokehTexUnit);
		glProgramUniform1i(bokehPass.program.id, 		bokehPass.program["ColorTex"].location,			bokehPass.colorTexUnit);

		gli::Image img;
		gli::io::Load("../resources/textures/HexaBokeh2.png",img);
//		gli::io::Load("../resources/textures/CircleBokeh.png",img);
		assert(img.Type()==gli::PixelFormat::NCHAR);
		switch(img.Format())
		{
			case gli::PixelFormat::R :
				bokehPass.bokehTex.Allocate(GL_R8,img.Width(),img.Height());
				bokehPass.bokehTex.Fill(GL_RED,GL_UNSIGNED_BYTE,img.Raw());
				break;
			case gli::PixelFormat::RG :
				bokehPass.bokehTex.Allocate(GL_RG8,img.Width(),img.Height());
				bokehPass.bokehTex.Fill(GL_RG,GL_UNSIGNED_BYTE,img.Raw());
				break;
			case gli::PixelFormat::RGB :
				bokehPass.bokehTex.Allocate(GL_RGB8,img.Width(),img.Height());
				bokehPass.bokehTex.Fill(GL_RGB,GL_UNSIGNED_BYTE,img.Raw());
				break;
			case gli::PixelFormat::RGBA :
				bokehPass.bokehTex.Allocate(GL_RGBA8,img.Width(),img.Height());
				bokehPass.bokehTex.Fill(GL_RGBA,GL_UNSIGNED_BYTE,img.Raw());
				break;
			default:
				assert(false);
				break;
		}

		glf::CheckError("DOFProcessor::Create");
	}
	//-------------------------------------------------------------------------
	void DOFProcessor::Draw(	const Texture2D& _inputTex, 
								const Texture2D& _positionTex, 
								const glm::mat4& _view,
								float 			_nearStart,
								float 			_nearEnd,
								float 			_farStart,
								float 			_farEnd,
								float 			_maxRadius,
								int 			_nSamples,
								float 			_intThreshold,
								float 			_cocThreshold,
								float 			_attenuation,
								float			_areaFactor)
	{
		// Reset the number of bokeh
		DrawArraysIndirectCommand* cmd = indirectBuffer.Lock();
		cmd[0].primCount = 0;
		indirectBuffer.Unlock();

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(cocPass.program.id);
		glProgramUniform1f(cocPass.program.id,			cocPass.intThresholdVar,	_intThreshold);
		glProgramUniform1f(cocPass.program.id,			cocPass.cocThresholdVar,	_cocThreshold);
		glProgramUniform1f(cocPass.program.id,			cocPass.nearStartVar,		_nearStart);
		glProgramUniform1f(cocPass.program.id,			cocPass.nearEndVar,			_nearEnd);
		glProgramUniform1f(cocPass.program.id,			cocPass.farStartVar,		_farStart);
		glProgramUniform1f(cocPass.program.id,			cocPass.farEndVar,			_farEnd);
		glProgramUniform1f(cocPass.program.id,			cocPass.maxRadiusVar,		_maxRadius);
		glProgramUniform1f(cocPass.program.id,			cocPass.areaFactorVar,		_areaFactor);
		glProgramUniform1i(cocPass.program.id,			cocPass.nSamplesVar,		_nSamples);
		glProgramUniformMatrix4fv(cocPass.program.id, 	cocPass.viewMatVar,			1, GL_FALSE, &_view[0][0]);

		glActiveTexture(GL_TEXTURE0 + cocPass.countTexUnit);
		glBindTexture(GL_TEXTURE_BUFFER, countTexID);
		glBindImageTextureEXT(cocPass.countTexUnit, 	countTexID, 	0, false, 0,  GL_READ_WRITE, GL_R32UI);
		glActiveTexture(GL_TEXTURE0 + cocPass.sampleTexUnit);
		glBindImageTextureEXT(cocPass.sampleTexUnit,	sampleTex.id,	0, false, 0,  GL_READ_WRITE, GL_RGBA32F);
		glActiveTexture(GL_TEXTURE0 + cocPass.colorTexUnit);
		glBindImageTextureEXT(cocPass.colorTexUnit, 	colorTex.id,	0, false, 0,  GL_READ_WRITE, GL_RGBA32F);
		_inputTex.Bind(cocPass.inputTexUnit);
		_positionTex.Bind(cocPass.positionTexUnit);
		cocPass.rotationTex.Bind(cocPass.rotationTexUnit);
		glBindBuffer(GL_ARRAY_BUFFER, quadbuffer.id);
		glVertexAttribPointer(	vbufferVar, 
								3, 
								GL_FLOAT, 
								false,
								sizeof(glm::vec3),
								GLF_BUFFER_OFFSET(0));
		glEnableVertexAttribArray(vbufferVar);
		glDrawArrays(GL_TRIANGLES, 0, 6);


		// Get number of bokeh point
		glMemoryBarrierEXT(GL_ALL_BARRIER_BITS_EXT);

//		int count, primCount,first,reservedMustBeZero;
//		DrawArraysIndirectCommand* indirectCmd = indirectBuffer.Lock();
//		count = indirectCmd[0].count;
//		primCount = indirectCmd[0].primCount;
//		first = indirectCmd[0].first;
//		reservedMustBeZero = indirectCmd[0].reservedMustBeZero;
//		indirectBuffer.Unlock();
//		std::stringstream out;
//		out << "count : " << count << "  ";
//		out << "primCount : " << primCount << "  ";
//		out << "first : " << first << "  ";
//		out << "reservedMustBeZero  : " << reservedMustBeZero << "\n";
//		glf::Info("%s",out.str().c_str());

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);
//glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(bokehPass.program.id);
		glProgramUniform1f(bokehPass.program.id,	bokehPass.attenuationVar,	_attenuation);
		bokehPass.bokehTex.Bind(bokehPass.bokehTexUnit);
		colorTex.Bind(bokehPass.colorTexUnit);
		sampleTex.Bind(bokehPass.sampleTexUnit);
		glBindBuffer(GL_ARRAY_BUFFER, pointbuffer.id);
		glVertexAttribPointer(	vbufferVar, 
								3, 
								GL_FLOAT, 
								false,
								sizeof(glm::vec3),
								GLF_BUFFER_OFFSET(0));
		glEnableVertexAttribArray(vbufferVar);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER,indirectBuffer.id);
		glDrawArraysIndirect(GL_POINTS,NULL);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER,0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glf::CheckError("DOFProcessor::Draw");
	}
	//-------------------------------------------------------------------------
	DOFProcessor::Ptr DOFProcessor::Create(int _w, int _h)
	{
		return DOFProcessor::Ptr(new DOFProcessor(_w,_h));
	}
}

