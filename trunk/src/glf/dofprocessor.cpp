//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/dofprocessor.hpp>
#include <glf/ioimage.hpp>
#include <glf/debug.hpp>
#include <glm/glm.hpp>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define RUN_TIMINGS 0

namespace glf
{
	//-------------------------------------------------------------------------
	DOFProcessor::DOFProcessor(int _w, int _h)
	{
		glm::mat4 transform	= ScreenQuadTransform();
		
		// Resources initialization
		{
			// Load bokeh texture
			BokehTexture("../resources/textures/HexaBokeh.png");

			// TODO : 16F
			blurDepthTex.Allocate(GL_RGBA32F,_w,_h);
			blurDepthTex.SetFiltering(GL_LINEAR,GL_LINEAR);
			detectionTex.Allocate(GL_RGBA32F,_w,_h);
			detectionTex.SetFiltering(GL_LINEAR,GL_LINEAR);
			blurTex.Allocate(GL_RGBA32F,_w,_h);
			blurTex.SetFiltering(GL_LINEAR,GL_LINEAR);

			glGenFramebuffers(1, &blurDepthFBO);
			glBindFramebuffer(GL_FRAMEBUFFER,blurDepthFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, blurDepthTex.target, blurDepthTex.id, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBindFramebuffer(GL_FRAMEBUFFER,0);
			glf::CheckFramebuffer(blurDepthFBO);

			glGenFramebuffers(1, &detectionFBO);
			glBindFramebuffer(GL_FRAMEBUFFER,detectionFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, detectionTex.target, detectionTex.id, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBindFramebuffer(GL_FRAMEBUFFER,0);
			glf::CheckFramebuffer(detectionFBO);

			glGenFramebuffers(1, &blurFBO);
			glBindFramebuffer(GL_FRAMEBUFFER,blurFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, blurTex.target, blurTex.id, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBindFramebuffer(GL_FRAMEBUFFER,0);
			glf::CheckFramebuffer(blurFBO);

			// Create texture for counting bokeh
			// Texture size is set to the resolution in order to avoid overflow
			bokehPositionTex.Allocate(GL_RGBA32F,_w,_h);
			bokehPositionTex.SetFiltering(GL_NEAREST,GL_NEAREST);
			bokehColorTex.Allocate(GL_RGBA32F,_w,_h);
			bokehColorTex.SetFiltering(GL_NEAREST,GL_NEAREST);

			// Setup the indirect buffer
			pointIndirectBuffer.Allocate(1);
			DrawArraysIndirectCommand* indirectCmd = pointIndirectBuffer.Lock();
			indirectCmd[0].count 				= 1;
			indirectCmd[0].primCount 			= 0; // Number of instance to draw
			indirectCmd[0].first 				= 0;
			indirectCmd[0].reservedMustBeZero 	= 0;
			pointIndirectBuffer.Unlock();

			// Create the texture proxy
			glGenTextures(1, &bokehCountTexID);
			glBindTexture(GL_TEXTURE_BUFFER, bokehCountTexID);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, pointIndirectBuffer.id);
			glBindTexture(GL_TEXTURE_BUFFER, 0);

			// Create point VBO and VAO
			pointVBO.Allocate(1,GL_STATIC_DRAW);
			glm::vec3* vertices = pointVBO.Lock();
			vertices[0] = glm::vec3(0,0,0);
			pointVBO.Unlock();
			pointVAO.Add(pointVBO,semantic::Position,3,GL_FLOAT);
		}

		// Reset Pass
		{
			resetPass.program.Compile(	LoadFile("../resources/shaders/bokehreset.vs"),
										LoadFile("../resources/shaders/bokehreset.fs"));

			resetPass.bokehCountTexUnit = resetPass.program["BokehCountTex"].unit;
			glProgramUniform1i(resetPass.program.id,resetPass.program["BokehCountTex"].location,resetPass.bokehCountTexUnit);
		}

		// CoC Pass
		{
			cocPass.program.Compile(LoadFile("../resources/shaders/bokehcoc.vs"),
									LoadFile("../resources/shaders/bokehcoc.fs"));

			cocPass.farStartVar			= cocPass.program["FarStart"].location;
			cocPass.farEndVar			= cocPass.program["FarEnd"].location;
			cocPass.viewMatVar			= cocPass.program["ViewMat"].location;
			cocPass.positionTexUnit		= cocPass.program["PositionTex"].unit;

			glProgramUniformMatrix4fv(cocPass.program.id, cocPass.program["Transformation"].location,1, GL_FALSE, &transform[0][0]);
			glProgramUniform1i(cocPass.program.id, 		  cocPass.program["PositionTex"].location,cocPass.positionTexUnit);
		}

		// Detection Pass
		{
			detectionPass.program.Compile(	LoadFile("../resources/shaders/bokehdetection.vs"),
											LoadFile("../resources/shaders/bokehdetection.fs"));

			detectionPass.colorTexUnit		= detectionPass.program["ColorTex"].unit;
			detectionPass.blurDepthTexUnit	= detectionPass.program["BlurDepthTex"].unit;
			detectionPass.lumThresholdVar	= detectionPass.program["LumThreshold"].location;
			detectionPass.cocThresholdVar	= detectionPass.program["CoCThreshold"].location;
			detectionPass.maxCoCRadiusVar	= detectionPass.program["MaxCoCRadius"].location;
			detectionPass.bokehCountTexUnit	= detectionPass.program["BokehCountTex"].unit;
			detectionPass.bokehColorTexUnit	= detectionPass.program["BokehColorTex"].unit;
			detectionPass.bokehPositionTexUnit= detectionPass.program["BokehPositionTex"].unit;

			glProgramUniformMatrix4fv(detectionPass.program.id, detectionPass.program["Transformation"].location,1,GL_FALSE,&transform[0][0]);
			glProgramUniform1i(detectionPass.program.id, detectionPass.program["BlurDepthTex"].location,detectionPass.blurDepthTexUnit);
			glProgramUniform1i(detectionPass.program.id, detectionPass.program["ColorTex"].location,detectionPass.colorTexUnit);
			glProgramUniform1i(detectionPass.program.id, detectionPass.program["BokehCountTex"].location,detectionPass.bokehCountTexUnit);
			glProgramUniform1i(detectionPass.program.id, detectionPass.program["BokehColorTex"].location,detectionPass.bokehColorTexUnit);
			glProgramUniform1i(detectionPass.program.id, detectionPass.program["BokehPositionTex"].location,detectionPass.bokehPositionTexUnit);
		}

		// Blur separable pass
		{
			blurSeparablePass.program.Compile(	LoadFile("../resources/shaders/bokehblur.vs"),
												LoadFile("../resources/shaders/bokehblur.fs"));

			blurSeparablePass.blurDepthTexUnit	= blurSeparablePass.program["BlurDepthTex"].unit;
			blurSeparablePass.colorTexUnit		= blurSeparablePass.program["ColorTex"].unit;
			blurSeparablePass.maxCoCRadiusVar	= blurSeparablePass.program["MaxCoCRadius"].location;
			blurSeparablePass.directionVar		= blurSeparablePass.program["Direction"].location;

			glProgramUniformMatrix4fv(blurSeparablePass.program.id, blurSeparablePass.program["Transformation"].location,1,GL_FALSE, &transform[0][0]);
			glProgramUniform1i(blurSeparablePass.program.id, 		blurSeparablePass.program["BlurDepthTex"].location,blurSeparablePass.blurDepthTexUnit);
			glProgramUniform1i(blurSeparablePass.program.id, 		blurSeparablePass.program["ColorTex"].location,blurSeparablePass.colorTexUnit);
		}

		// Blur poisson pass
		{
			// Sampling point
			glm::vec2 Halton[32];
			Halton[0]       = glm::vec2(-0.353553, 0.612372);
			Halton[1]       = glm::vec2(-0.25, -0.433013);
			Halton[2]       = glm::vec2(0.663414, 0.55667);
			Halton[3]       = glm::vec2(-0.332232, 0.120922);
			Halton[4]       = glm::vec2(0.137281, -0.778559);
			Halton[5]       = glm::vec2(0.106337, 0.603069);
			Halton[6]       = glm::vec2(-0.879002, -0.319931);
			Halton[7]       = glm::vec2(0.191511, -0.160697);
			Halton[8]       = glm::vec2(0.729784, 0.172962);
			Halton[9]       = glm::vec2(-0.383621, 0.406614);
			Halton[10]      = glm::vec2(-0.258521, -0.86352);
			Halton[11]      = glm::vec2(0.258577, 0.34733);
			Halton[12]      = glm::vec2(-0.82355, 0.0962588);
			Halton[13]      = glm::vec2(0.261982, -0.607343);
			Halton[14]      = glm::vec2(-0.0562987, 0.966608);
			Halton[15]      = glm::vec2(-0.147695, -0.0971404);
			Halton[16]      = glm::vec2(0.651341, -0.327115);
			Halton[17]      = glm::vec2(0.47392, 0.238012);
			Halton[18]      = glm::vec2(-0.738474, 0.485702);
			Halton[19]      = glm::vec2(-0.0229837, -0.394616);
			Halton[20]      = glm::vec2(0.320861, 0.74384);
			Halton[21]      = glm::vec2(-0.633068, -0.0739953);
			Halton[22]      = glm::vec2(0.568478, -0.763598);
			Halton[23]      = glm::vec2(-0.0878153, 0.293323);
			Halton[24]      = glm::vec2(-0.528785, -0.560479);
			Halton[25]      = glm::vec2(0.570498, -0.13521);
			Halton[26]      = glm::vec2(0.915797, 0.0711813);
			Halton[27]      = glm::vec2(-0.264538, 0.385706);
			Halton[28]      = glm::vec2(-0.365725, -0.76485);
			Halton[29]      = glm::vec2(0.488794, 0.479406);
			Halton[30]      = glm::vec2(-0.948199, 0.263949);
			Halton[31]      = glm::vec2(0.0311802, -0.121049);

			blurPoissonPass.program.Compile(	LoadFile("../resources/shaders/bokehblurpoisson.vs"),
												LoadFile("../resources/shaders/bokehblurpoisson.fs"));

			blurPoissonPass.blurDepthTexUnit	= blurPoissonPass.program["BlurDepthTex"].unit;
			blurPoissonPass.colorTexUnit		= blurPoissonPass.program["ColorTex"].unit;
			blurPoissonPass.maxCoCRadiusVar		= blurPoissonPass.program["MaxCoCRadius"].location;
			blurPoissonPass.nSamplesVar			= blurPoissonPass.program["NSamples"].location;

			glProgramUniformMatrix4fv(blurPoissonPass.program.id, blurPoissonPass.program["Transformation"].location,1,GL_FALSE, &transform[0][0]);
			glProgramUniform1i(blurPoissonPass.program.id,		  blurPoissonPass.program["BlurDepthTex"].location,blurPoissonPass.blurDepthTexUnit);
			glProgramUniform1i(blurPoissonPass.program.id,		  blurPoissonPass.program["ColorTex"].location,blurPoissonPass.colorTexUnit);
			glProgramUniform2fv(blurPoissonPass.program.id,		  blurPoissonPass.program["Samples"].location,32,&Halton[0][0]);
		}

		// Rendering pass
		{
			renderingPass.program.Compile(	LoadFile("../resources/shaders/bokehrendering.vs"),
											LoadFile("../resources/shaders/bokehrendering.gs"),
											LoadFile("../resources/shaders/bokehrendering.fs"));

			renderingPass.blurDepthTexUnit		= renderingPass.program["BlurDepthTex"].unit;
			renderingPass.bokehPositionTexUnit	= renderingPass.program["BokehPositionTex"].unit;
			renderingPass.bokehColorTexUnit		= renderingPass.program["BokehColorTex"].unit;
			renderingPass.bokehShapeTexUnit		= renderingPass.program["BokehShapeTex"].unit;
			renderingPass.maxBokehRadiusVar		= renderingPass.program["MaxBokehRadius"].location;
			renderingPass.bokehDepthCutoffVar	= renderingPass.program["BokehDepthCutoff"].location;

			glProgramUniformMatrix4fv(renderingPass.program.id,	renderingPass.program["Transformation"].location,1, GL_FALSE, &transform[0][0]);
			glProgramUniform2f(renderingPass.program.id, renderingPass.program["PixelScale"].location,1.f/_w, 1.f/_h);
			glProgramUniform1i(renderingPass.program.id, renderingPass.program["BokehPositionTex"].location,renderingPass.bokehPositionTexUnit);
			glProgramUniform1i(renderingPass.program.id, renderingPass.program["BokehShapeTex"].location,renderingPass.bokehShapeTexUnit);
			glProgramUniform1i(renderingPass.program.id, renderingPass.program["BokehColorTex"].location,renderingPass.bokehColorTexUnit);
			glProgramUniform1i(renderingPass.program.id, renderingPass.program["BlurDepthTex"].location,renderingPass.blurDepthTexUnit);
		}

		glf::CheckError("DOFProcessor::Create");
	}
	//-------------------------------------------------------------------------
	void DOFProcessor::BokehTexture(		const std::string& _filename)
	{
		io::LoadTexture(_filename,
						bokehShapeTex,
						true,
						true);
	}
	//-------------------------------------------------------------------------
	void DOFProcessor::Draw(	const Texture2D& _colorTex, 
								const Texture2D& _positionTex, 
								const glm::mat4& _view,
								float			_nearStart,
								float			_nearEnd,
								float			_farStart,
								float			_farEnd,
								float 			_maxCoCRadius,
								float 			_maxBokehRadius,
								int				_nSamples,
								float			_lumThreshold,
								float			_cocThreshold,
								float			_bokehDepthCutoff,
								bool 			_poissonFiltering,
								const RenderTarget& _renderTarget)
	{
		glf::CheckError("DOFProcessor::DrawBegin");

		// Reset bokeh counter (draw a fake point)
		glf::manager::timings->StartSection(section::DofReset);
		glUseProgram(resetPass.program.id);
			glBindFramebuffer(GL_FRAMEBUFFER,blurDepthFBO);
			glActiveTexture(GL_TEXTURE0 + resetPass.bokehCountTexUnit);
			glBindImageTextureEXT(resetPass.bokehCountTexUnit, bokehCountTexID,0,false,0,GL_WRITE_ONLY, GL_R32UI);
			pointVAO.Draw(GL_POINTS,1,0);
		glf::manager::timings->EndSection(section::DofReset);

		// Compute amount of blur and linear depth for each pixel
		glf::manager::timings->StartSection(section::DofBlurDepth);
		glUseProgram(cocPass.program.id);
			glClear(GL_COLOR_BUFFER_BIT);
			glProgramUniform1f(cocPass.program.id,			cocPass.farStartVar,	_farStart);
			glProgramUniform1f(cocPass.program.id,			cocPass.farEndVar,		_farEnd);
			glProgramUniformMatrix4fv(cocPass.program.id,	cocPass.viewMatVar,		1, GL_FALSE, &_view[0][0]);
			_positionTex.Bind(cocPass.positionTexUnit);
			_renderTarget.Draw();
			glf::CheckError("DOFProcessor::DrawBLURDEPTH");
		glf::manager::timings->EndSection(section::DofBlurDepth);

		// Detect pixel which are bokeh and output color of pixels which are not bokeh
		glf::manager::timings->StartSection(section::DofDetection);
		glUseProgram(detectionPass.program.id);
			glBindFramebuffer(GL_FRAMEBUFFER,detectionFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			glProgramUniform1f(detectionPass.program.id,detectionPass.cocThresholdVar,_cocThreshold);
			glProgramUniform1f(detectionPass.program.id,detectionPass.lumThresholdVar,_lumThreshold);
			glProgramUniform1f(detectionPass.program.id,detectionPass.maxCoCRadiusVar,_maxCoCRadius);

			glActiveTexture(GL_TEXTURE0 + detectionPass.bokehCountTexUnit);
			glBindImageTextureEXT(detectionPass.bokehCountTexUnit, bokehCountTexID,0,false,0,GL_READ_WRITE, GL_R32UI);
			glActiveTexture(GL_TEXTURE0 + detectionPass.bokehPositionTexUnit);
			glBindImageTextureEXT(detectionPass.bokehPositionTexUnit, bokehPositionTex.id,0,false,0,GL_WRITE_ONLY,GL_RGBA32F);
			glActiveTexture(GL_TEXTURE0 + detectionPass.bokehColorTexUnit);
			glBindImageTextureEXT(detectionPass.bokehColorTexUnit, bokehColorTex.id,0,false,0,GL_WRITE_ONLY,GL_RGBA32F);

			blurDepthTex.Bind(detectionPass.blurDepthTexUnit);
			_colorTex.Bind(detectionPass.colorTexUnit);
			_renderTarget.Draw();
			glf::CheckError("DOFProcessor::DrawDETECTION");
		glf::manager::timings->EndSection(section::DofDetection);

			// Print indirect buffer
			#if 0
			int count, primCount,first,reservedMustBeZero;
			DrawArraysIndirectCommand* indirectCmd = pointIndirectBuffer.Lock();
				count              = indirectCmd[0].count;
				primCount          = indirectCmd[0].primCount;
				first              = indirectCmd[0].first;
				reservedMustBeZero = indirectCmd[0].reservedMustBeZero;
			pointIndirectBuffer.Unlock();
			std::stringstream out;
			out << "count : " << count << "  ";
			out << "primCount : " << primCount << "  ";
			out << "first : " << first << "  ";
			out << "reservedMustBeZero  : " << reservedMustBeZero;
			glf::Info("%s",out.str().c_str());
			#endif

		glf::manager::timings->StartSection(section::DofBlur);
		if(_poissonFiltering)
		{
		glUseProgram(blurPoissonPass.program.id);
			glBindFramebuffer(GL_FRAMEBUFFER,_renderTarget.framebuffer);
			glClear(GL_COLOR_BUFFER_BIT);
			glProgramUniform1f(blurPoissonPass.program.id,		blurPoissonPass.maxCoCRadiusVar,	_maxCoCRadius);
			glProgramUniform1i(blurPoissonPass.program.id,		blurPoissonPass.nSamplesVar,		_nSamples);
			blurDepthTex.Bind(blurPoissonPass.blurDepthTexUnit);
			detectionTex.Bind(blurPoissonPass.colorTexUnit);
			_renderTarget.Draw();
			glf::CheckError("DOFProcessor::DrawPOISSONBLUR");
		}
		else
		{
		// Vertical blur of pixels which are not bokehs
		glUseProgram(blurSeparablePass.program.id);
			glBindFramebuffer(GL_FRAMEBUFFER,blurFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			glProgramUniform1f(blurSeparablePass.program.id,		blurSeparablePass.maxCoCRadiusVar,	_maxCoCRadius);
			glProgramUniform2f(blurSeparablePass.program.id,		blurSeparablePass.directionVar,		1,0);
			blurDepthTex.Bind(blurSeparablePass.blurDepthTexUnit);
			detectionTex.Bind(blurSeparablePass.colorTexUnit);
			_renderTarget.Draw();
			glf::CheckError("DOFProcessor::DrawVBLUR");

		// Horizontal blur of pixels which are not bokehs
			glBindFramebuffer(GL_FRAMEBUFFER,_renderTarget.framebuffer);
			glClear(GL_COLOR_BUFFER_BIT);
			glProgramUniform1f(blurSeparablePass.program.id,		blurSeparablePass.maxCoCRadiusVar,	_maxCoCRadius);
			glProgramUniform2f(blurSeparablePass.program.id,		blurSeparablePass.directionVar,		0,1);
			blurDepthTex.Bind(blurSeparablePass.blurDepthTexUnit);
			blurTex.Bind(blurSeparablePass.colorTexUnit);
			_renderTarget.Draw();
			glf::CheckError("DOFProcessor::DrawHBLUR");
		}
		glf::manager::timings->EndSection(section::DofBlur);

		// Render bokeh as textured quad (with additive blending)
		glf::manager::timings->StartSection(section::DofRendering);
		glUseProgram(renderingPass.program.id);
			glMemoryBarrierEXT(GL_ALL_BARRIER_BITS_EXT);
			glProgramUniform1f(renderingPass.program.id,renderingPass.maxBokehRadiusVar,_maxBokehRadius);
			glProgramUniform1f(renderingPass.program.id,renderingPass.bokehDepthCutoffVar,_bokehDepthCutoff);
			bokehShapeTex.Bind(renderingPass.bokehShapeTexUnit);
			bokehColorTex.Bind(renderingPass.bokehColorTexUnit);
			blurDepthTex.Bind(renderingPass.blurDepthTexUnit);
			bokehPositionTex.Bind(renderingPass.bokehPositionTexUnit);
			pointVAO.Draw(GL_POINTS,pointIndirectBuffer);
			glf::CheckError("DOFProcessor::DrawRENDERING");
		glf::manager::timings->EndSection(section::DofRendering);

		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glf::CheckError("DOFProcessor::DrawEnd");
	}
}

