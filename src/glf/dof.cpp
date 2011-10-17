//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/dof.hpp>
#include <glm/glm.hpp>
#include <glf/rng.hpp>
#include <glf/ioimage.hpp>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

namespace glf
{
	//-------------------------------------------------------------------------
	DOFProcessor::DOFProcessor(int _w, int _h):
	maxBokehCount(_w*_h)
	{
		glm::mat4 transform	= ScreenQuadTransform();

		// Create point VBO and VAO
		pointVBO.Allocate(1,GL_STATIC_DRAW);
		glm::vec3* vertices = pointVBO.Lock();
		vertices[0] = glm::vec3(0,0,0);
		pointVBO.Unlock();
		pointVAO.Add(pointVBO,semantic::Position,3,GL_FLOAT);

		// Create texture for counting bokeh
		// Texture size is set to the resolution in order to avoid overflow
		bokehPosTex.Allocate(GL_RGBA32F,_w,_h);
		bokehPosTex.SetFiltering(GL_NEAREST,GL_NEAREST);
		bokehColorTex.Allocate(GL_RGBA32F,_w,_h);
		bokehColorTex.SetFiltering(GL_NEAREST,GL_NEAREST);

		// Setup the indirect buffer
		pointIndirectBuffer.Allocate(1);
		DrawArraysIndirectCommand* indirectCmd = pointIndirectBuffer.Lock();
		indirectCmd[0].count 				= 1;
		indirectCmd[0].primCount 			= 6;
		indirectCmd[0].first 				= 0;
		indirectCmd[0].reservedMustBeZero 	= 0;
		pointIndirectBuffer.Unlock();

		// Create the texture proxy
		glGenTextures(1, &bokehCountTexID);
		glBindTexture(GL_TEXTURE_BUFFER, bokehCountTexID);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, pointIndirectBuffer.id);

		// CoC Pass
		cocPass.program.Compile(LoadFile("../resources/shaders/dof.vs"),
								LoadFile("../resources/shaders/dof.fs"));

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
		cocPass.bokehPosTexUnit		= cocPass.program["BokehPosTex"].unit;
		cocPass.bokehColorTexUnit	= cocPass.program["BokehColorTex"].unit;
		cocPass.bokehCountTexUnit	= cocPass.program["BokehCountTex"].unit;

		glProgramUniformMatrix4fv(cocPass.program.id, cocPass.program["Transformation"].location,	1, GL_FALSE, &transform[0][0]);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["PositionTex"].location,		cocPass.positionTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["RotationTex"].location,		cocPass.rotationTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["InputTex"].location,			cocPass.inputTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["BokehPosTex"].location,		cocPass.bokehPosTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["BokehColorTex"].location,	cocPass.bokehColorTexUnit);
		glProgramUniform1i(cocPass.program.id, 		  cocPass.program["BokehCountTex"].location,	cocPass.bokehCountTexUnit);

		// Sampling point
		glm::vec2 Halton[32];
		Halton[0]	= glm::vec2(-0.353553, 0.612372);
		Halton[1]	= glm::vec2(-0.25, -0.433013);
		Halton[2]	= glm::vec2(0.663414, 0.55667);
		Halton[3]	= glm::vec2(-0.332232, 0.120922);
		Halton[4]	= glm::vec2(0.137281, -0.778559);
		Halton[5]	= glm::vec2(0.106337, 0.603069);
		Halton[6]	= glm::vec2(-0.879002, -0.319931);
		Halton[7]	= glm::vec2(0.191511, -0.160697);
		Halton[8]	= glm::vec2(0.729784, 0.172962);
		Halton[9]	= glm::vec2(-0.383621, 0.406614);
		Halton[10]	= glm::vec2(-0.258521, -0.86352);
		Halton[11]	= glm::vec2(0.258577, 0.34733);
		Halton[12]	= glm::vec2(-0.82355, 0.0962588);
		Halton[13]	= glm::vec2(0.261982, -0.607343);
		Halton[14]	= glm::vec2(-0.0562987, 0.966608);
		Halton[15]	= glm::vec2(-0.147695, -0.0971404);
		Halton[16]	= glm::vec2(0.651341, -0.327115);
		Halton[17]	= glm::vec2(0.47392, 0.238012);
		Halton[18]	= glm::vec2(-0.738474, 0.485702);
		Halton[19]	= glm::vec2(-0.0229837, -0.394616);
		Halton[20]	= glm::vec2(0.320861, 0.74384);
		Halton[21]	= glm::vec2(-0.633068, -0.0739953);
		Halton[22]	= glm::vec2(0.568478, -0.763598);
		Halton[23]	= glm::vec2(-0.0878153, 0.293323);
		Halton[24]	= glm::vec2(-0.528785, -0.560479);
		Halton[25]	= glm::vec2(0.570498, -0.13521);
		Halton[26]	= glm::vec2(0.915797, 0.0711813);
		Halton[27]	= glm::vec2(-0.264538, 0.385706);
		Halton[28]	= glm::vec2(-0.365725, -0.76485);
		Halton[29]	= glm::vec2(0.488794, 0.479406);
		Halton[30]	= glm::vec2(-0.948199, 0.263949);
		Halton[31]	= glm::vec2(0.0311802, -0.121049);
		glProgramUniform2fv(cocPass.program.id, cocPass.program["Halton"].location, 32, &Halton[0][0]);

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

		bokehPass.bokehPosTexUnit	= bokehPass.program["BokehPosTex"].unit;
		bokehPass.bokehColorTexUnit	= bokehPass.program["BokehColorTex"].unit;
		bokehPass.bokehShapeTexUnit	= bokehPass.program["BokehShapeTex"].unit;
		bokehPass.attenuationVar	= bokehPass.program["Attenuation"].location;

		glProgramUniformMatrix4fv(bokehPass.program.id,	bokehPass.program["Transformation"].location,	1, GL_FALSE, &transform[0][0]);
		glProgramUniform2f(bokehPass.program.id, 		bokehPass.program["PixelScale"].location,		1.f/_w, 1.f/_h);
		glProgramUniform1i(bokehPass.program.id, 		bokehPass.program["BokehPosTex"].location,		bokehPass.bokehPosTexUnit);
		glProgramUniform1i(bokehPass.program.id, 		bokehPass.program["BokehShapeTex"].location,	bokehPass.bokehShapeTexUnit);
		glProgramUniform1i(bokehPass.program.id, 		bokehPass.program["BokehColorTex"].location,	bokehPass.bokehColorTexUnit);

		// Load bokeh texture
		//io::LoadTexture("../resources/textures/CircleBokeh.png",
		io::LoadTexture("../resources/textures/HexaBokeh.png",
						bokehPass.bokehShapeTex,
						true,
						true);

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
								float			_areaFactor,
								const RenderTarget& _renderTarget)
	{
		// Reset the number of bokeh TODO : do it with a shader
		DrawArraysIndirectCommand* cmd = pointIndirectBuffer.Lock();
		cmd[0].primCount = 0;
		pointIndirectBuffer.Unlock();

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

		glActiveTexture(GL_TEXTURE0 + cocPass.bokehCountTexUnit);
		glBindTexture(GL_TEXTURE_BUFFER, bokehCountTexID);
		glBindImageTextureEXT(cocPass.bokehCountTexUnit, 	bokehCountTexID,	0, false, 0,  GL_READ_WRITE, GL_R32UI);
		glActiveTexture(GL_TEXTURE0 + cocPass.bokehPosTexUnit);
		glBindImageTextureEXT(cocPass.bokehPosTexUnit,		bokehPosTex.id, 	0, false, 0,  GL_READ_WRITE, GL_RGBA32F);
		glActiveTexture(GL_TEXTURE0 + cocPass.bokehColorTexUnit);
		glBindImageTextureEXT(cocPass.bokehColorTexUnit, 	bokehColorTex.id,	0, false, 0,  GL_READ_WRITE, GL_RGBA32F);
		_inputTex.Bind(cocPass.inputTexUnit);
		_positionTex.Bind(cocPass.positionTexUnit);
		cocPass.rotationTex.Bind(cocPass.rotationTexUnit);
		_renderTarget.Draw();


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


		glUseProgram(bokehPass.program.id);
		glProgramUniform1f(bokehPass.program.id,bokehPass.attenuationVar,_attenuation);
		bokehPass.bokehShapeTex.Bind(bokehPass.bokehShapeTexUnit);
		bokehColorTex.Bind(bokehPass.bokehColorTexUnit);
		bokehPosTex.Bind(bokehPass.bokehPosTexUnit);
		pointVAO.Draw(GL_POINTS,pointIndirectBuffer);

		glf::CheckError("DOFProcessor::Draw");
	}
}

