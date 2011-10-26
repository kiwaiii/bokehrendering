//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/dof2.hpp>
#include <glm/glm.hpp>
#include <glf/rng.hpp>
#include <glf/ioimage.hpp>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

namespace glf
{
	//-------------------------------------------------------------------------
	DOFProcessor::DOFProcessor(int _w, int _h)
	{
		glm::mat4 transform	= ScreenQuadTransform();
		
		// Resources initialization
		{
			// Load bokeh texture
			//io::LoadTexture("../resources/textures/CircleBokeh.png",
			io::LoadTexture("../resources/textures/HexaBokeh.png",
							bokehShapeTex,
							true,
							true);

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

		// Blur pass
		{
			blurPass.program.Compile(	LoadFile("../resources/shaders/bokehblur.vs"),
										LoadFile("../resources/shaders/bokehblur.fs"));

			blurPass.blurDepthTexUnit	= blurPass.program["BlurDepthTex"].unit;
			blurPass.colorTexUnit		= blurPass.program["ColorTex"].unit;
			blurPass.maxCoCRadiusVar	= blurPass.program["MaxCoCRadius"].location;
			blurPass.directionVar		= blurPass.program["Direction"].location;

			glProgramUniformMatrix4fv(blurPass.program.id,  blurPass.program["Transformation"].location,1,GL_FALSE, &transform[0][0]);
			glProgramUniform1i(blurPass.program.id, 		blurPass.program["BlurDepthTex"].location,blurPass.blurDepthTexUnit);
			glProgramUniform1i(blurPass.program.id, 		blurPass.program["ColorTex"].location,blurPass.colorTexUnit);
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
								const RenderTarget& _renderTarget)
	{
		glf::CheckError("DOFProcessor::DrawBegin");

		// Reset 
		// Reset the number of bokeh TODO : do it with a shader
		DrawArraysIndirectCommand* cmd = pointIndirectBuffer.Lock();
		cmd[0].primCount = 0;
		pointIndirectBuffer.Unlock();

		// Blur / Depth
		glUseProgram(cocPass.program.id);
			glBindFramebuffer(GL_FRAMEBUFFER,blurDepthFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			glProgramUniform1f(cocPass.program.id,			cocPass.farStartVar,	_farStart);
			glProgramUniform1f(cocPass.program.id,			cocPass.farEndVar,		_farEnd);
			glProgramUniformMatrix4fv(cocPass.program.id,	cocPass.viewMatVar,		1, GL_FALSE, &_view[0][0]);
			_positionTex.Bind(cocPass.positionTexUnit);
			_renderTarget.Draw();
			glf::CheckError("DOFProcessor::DrawBLURDEPTH");

		// Bokeh Extraction 
		glUseProgram(detectionPass.program.id);
			glBindFramebuffer(GL_FRAMEBUFFER,detectionFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			glProgramUniform1f(detectionPass.program.id,detectionPass.cocThresholdVar,_cocThreshold);
			glProgramUniform1f(detectionPass.program.id,detectionPass.lumThresholdVar,_lumThreshold);
			glProgramUniform1f(detectionPass.program.id,detectionPass.maxCoCRadiusVar,_maxCoCRadius);

			// TODO write only for Position and Color
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
			out << "reservedMustBeZero  : " << reservedMustBeZero << "\n";
			glf::Info("%s",out.str().c_str());
			#endif

		// V-Blur
		glUseProgram(blurPass.program.id);
			glBindFramebuffer(GL_FRAMEBUFFER,blurFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			glProgramUniform1f(blurPass.program.id,		blurPass.maxCoCRadiusVar,	_maxCoCRadius);
			glProgramUniform2f(blurPass.program.id,		blurPass.directionVar,		1,0);
			blurDepthTex.Bind(blurPass.blurDepthTexUnit);
			detectionTex.Bind(blurPass.colorTexUnit);
			_renderTarget.Draw();
			glf::CheckError("DOFProcessor::DrawVBLUR");

		// H-Blur
			glBindFramebuffer(GL_FRAMEBUFFER,_renderTarget.framebuffer);
			glClear(GL_COLOR_BUFFER_BIT);
			glProgramUniform1f(blurPass.program.id,		blurPass.maxCoCRadiusVar,	_maxCoCRadius);
			glProgramUniform2f(blurPass.program.id,		blurPass.directionVar,		0,1);
			blurDepthTex.Bind(blurPass.blurDepthTexUnit);
			blurTex.Bind(blurPass.colorTexUnit);
			_renderTarget.Draw();
			glf::CheckError("DOFProcessor::DrawHBLUR");

		// Bokeh rendering (with additive blending)
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

		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glf::CheckError("DOFProcessor::DrawEnd");
	}
}

