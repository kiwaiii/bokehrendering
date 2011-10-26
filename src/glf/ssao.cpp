//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/ssao.hpp>
#include <glf/rng.hpp>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

namespace glf
{
	//-------------------------------------------------------------------------
	SSAOPass::SSAOPass(int _w, int _h):
	program("SSAOPass")
	{
		program.Compile(LoadFile("../resources/shaders/ssao.vs"),
						LoadFile("../resources/shaders/ssao.fs"));

		betaVar				= program["Beta"].location;
		epsilonVar			= program["Epsilon"].location;
		kappaVar	 		= program["Kappa"].location;
		sigmaVar			= program["Sigma"].location;
		radiusVar			= program["Radius"].location;
		nSamplesVar			= program["nSamples"].location;
		viewMatVar			= program["View"].location;
		nearVar				= program["Near"].location;

		positionTexUnit		= program["PositionTex"].unit;
		normalTexUnit		= program["NormalTex"].unit;
		rotationTexUnit		= program["RotationTex"].unit;

		glm::mat4 transform	= ScreenQuadTransform();
		glProgramUniformMatrix4fv(program.id, program["Transformation"].location,	1, GL_FALSE, &transform[0][0]);
		glProgramUniform1i(program.id, 		  program["PositionTex"].location,		positionTexUnit);
		glProgramUniform1i(program.id, 		  program["NormalTex"].location,		normalTexUnit);
		glProgramUniform1i(program.id, 		  program["RotationTex"].location,		rotationTexUnit);

		// Create and fill rotation texture
		RNG rng;
		rotationTex.Allocate(GL_RG32F,_w,_h);
		glm::vec2* rotations = new glm::vec2[_w * _h];

		for(int y=0;y<_h;++y)
		for(int x=0;x<_w;++x)
		{	
			float theta 		= 2.f * M_PI * rng.RandomFloat();
			rotations[x+y*_w] 	= glm::vec2(cos(theta),sin(theta));
		}
		rotationTex.Fill(GL_RG,GL_FLOAT,(unsigned char*)rotations);
		delete[] rotations;

		// Halton sequence generated using: WONG, T.-T., LUK, W.-S., AND HENG, P.-A. 1997.
		// Sampling with hammersley and Halton points
		// http://www.cse.cuhk.edu.hk/~ttwong/papers/udpoint/udpoint.pdf
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
		glProgramUniform2fv(program.id, program["Halton"].location,	32, &Halton[0][0]);

		glf::CheckError("SSAOPass::Create");
	}
	//-------------------------------------------------------------------------
	void SSAOPass::Draw(	const GBuffer&	_gbuffer,
							const glm::mat4& _view,
							float 			_near,
							float 			_beta,
							float 			_epsilon,
							float 			_kappa,
							float 			_sigma,
							float 			_radius,
							int 			_nSamples,
							const RenderTarget& _renderTarget)
	{
		glUseProgram(program.id);
		glProgramUniform1f(program.id,			nearVar,			_near);
		glProgramUniform1f(program.id,			betaVar,			_beta);
		glProgramUniform1f(program.id,			epsilonVar,			_epsilon);
		glProgramUniform1f(program.id,			kappaVar,			_kappa);
		glProgramUniform1f(program.id,			sigmaVar,			_sigma);
		glProgramUniform1f(program.id,			radiusVar,			_radius);
		glProgramUniform1i(program.id,			nSamplesVar,		_nSamples);
		glProgramUniformMatrix4fv(program.id, 	viewMatVar,	1, GL_FALSE, &_view[0][0]);

		_gbuffer.positionTex.Bind(positionTexUnit);
		_gbuffer.normalTex.Bind(normalTexUnit);
		rotationTex.Bind(rotationTexUnit);
		_renderTarget.Draw();

		glf::CheckError("SSAOPass::Draw");
	}
	//-------------------------------------------------------------------------
	BilateralPass::BilateralPass(int _w, int _h):
	program("BilateralPass")
	{
		program.Compile(LoadFile("../resources/shaders/bilateral.vs"),
						LoadFile("../resources/shaders/bilateral.fs"));

		sigmaHVar			= program["SigmaH"].location;
		sigmaVVar			= program["SigmaV"].location;
		nTapsVar			= program["nTaps"].location;
		viewMatVar			= program["ViewMat"].location;

		positionTexUnit		= program["PositionTex"].unit;
		inputTexUnit		= program["InputTex"].unit;

		glm::mat4 transform	= ScreenQuadTransform();

		glProgramUniform1i(program.id, 		  program["InputTex"].location,				inputTexUnit);
		glProgramUniform1i(program.id, 		  program["PositionTex"].location,			positionTexUnit);
		glProgramUniformMatrix4fv(program.id, program["Transformation"].location,		1, GL_FALSE, &transform[0][0]);

		glf::CheckError("BilateralPass::Create");
	}
	//-------------------------------------------------------------------------
	void BilateralPass::Draw(	const Texture2D& _inputTex,
								const Texture2D& _positionTex,
								const glm::mat4& _view,
								float 			 _sigmaH,
								float 			 _sigmaV,
								int 			 _nTaps,
								const RenderTarget& _renderTarget)
	{
		glUseProgram(program.id);
		glProgramUniform1f(program.id,			sigmaHVar,			_sigmaH);
		glProgramUniform1f(program.id,			sigmaVVar,			_sigmaV);
		glProgramUniform1i(program.id,			nTapsVar,			_nTaps);
		glProgramUniformMatrix4fv(program.id, 	viewMatVar,			1, GL_FALSE, &_view[0][0]);

		_inputTex.Bind(inputTexUnit);
		_positionTex.Bind(positionTexUnit);
		_renderTarget.Draw();

		glf::CheckError("BilateralPass::Draw");
	}
	//-------------------------------------------------------------------------
}

