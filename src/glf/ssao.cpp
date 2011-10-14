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
		program.Compile(LoadFile("../resources/shaders/ssaopass.vs"),
						LoadFile("../resources/shaders/ssaopass.fs"));

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

