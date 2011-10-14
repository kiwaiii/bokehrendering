#ifndef GLF_SSAO_HPP
#define GLF_SSAO_HPP

//------------------------------------------------------------------------------
// Include
//------------------------------------------------------------------------------
#include <glf/wrapper.hpp>
#include <glf/pass.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	class BilateralPass
	{
	private:
					BilateralPass(	const BilateralPass&);
		BilateralPass operator=(	const BilateralPass&);
	public:
					BilateralPass(	int _w, 
									int _h);
		void 		Draw(			const Texture2D& _inputTex,
									const Texture2D& _positionTex,
									const glm::mat4& _viewMat,
									float 			 _sigmaH,
									float 			 _sigmaV,
									int 			 _nTaps,
									const RenderTarget& _renderTarget);
	public:
		GLint 						positionTexUnit;
		GLint 						inputTexUnit;

		GLint						viewMatVar;
		GLint						sigmaHVar;
		GLint						sigmaVVar;
		GLint						nTapsVar;

		Program 					program;
	};
	//--------------------------------------------------------------------------
	class SSAOPass
	{
	private:
					SSAOPass(		const SSAOPass&);
		SSAOPass	operator=(		const SSAOPass&);
	public:
					SSAOPass(		int _w, 
									int _h);
		void 		Draw(			const GBuffer&	_gbuffer,
									const glm::mat4& _view,
									float 			_near,
									float 			_beta,
									float 			_epsilon,
									float 			_kappa,
									float			_sigma,
									float			_radius,
									int 			_nSamples,
									const RenderTarget& _renderTarget);
	public:
		GLint 						positionTexUnit;
		GLint 						normalTexUnit;
		GLint						rotationTexUnit;

		GLint						nearVar;
		GLint						betaVar;
		GLint						epsilonVar;
		GLint						kappaVar;
		GLint						sigmaVar;
		GLint						radiusVar;
		GLint						nSamplesVar;
		GLint						viewMatVar;

		Program 					program;
		Texture2D					rotationTex;
	};
	//--------------------------------------------------------------------------
}

#endif
