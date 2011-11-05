#ifndef GLF_SKY_HPP
#define GLF_SKY_HPP

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glm/glm.hpp>
#include <glf/wrapper.hpp>
#include <glf/texture.hpp>
#include <glf/buffer.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	class CubeMap
	{
	private:
 					CubeMap(			const CubeMap&);
 		CubeMap&	operator=(			const CubeMap&);
	public:
					CubeMap(			);
				   ~CubeMap(			);
		void		Draw(				const glm::mat4& 		_proj, 
										const glm::mat4& 		_view,
										const TextureArray2D& 	_envTex);

		Program							program;
		GLint							transformVar;
		GLint							envTexUnit;
		VertexBuffer3F					vbuffer;
		VertexBuffer3F					tbuffer;
		VertexArray						vao;
	};
	//--------------------------------------------------------------------------
	class SkyBuilder
	{
	private:
 					SkyBuilder(			const SkyBuilder&);
 		SkyBuilder& operator=(			const SkyBuilder&);
	public:
					SkyBuilder(			int _resolution);
				   ~SkyBuilder();
		void		SetSunFactor(		float _factor);
		void		SetPosition(		float _theta,
										float _phi);
		void		SetTurbidity(		float _turbidity);
		void		Update(				bool _drawSun=true);

		GLuint							skyFramebuffer;
		TextureArray2D					skyTexture;
		Program							program;
		int								resolution;
		float							sunTheta;
		float							sunPhi;
		float 							sunFactor;
		glm::vec3						sunIntensity;
		float							turbidity;
		GLint							sunSphCoordVar;
		GLint							turbidityVar;
		GLint							sunFactorVar;
		GLint							drawSunVar;
		VertexBuffer3F					vbo;
		VertexArray						vao;
	private:
		glm::vec3 	ToCartesian(		float _theta, 
										float _phi);
		float 		PerezFunction(		float A,  
										float B,  
										float C,  
										float D,  
										float E,  
										float cosTheta, 
										float gamma,
										float thetaS, 
										float lvz );
		glm::vec3 	ComputeSunIntensity(float _sunTheta, 
										float _sunPhi, 
										float _turbidity);
	};
	//--------------------------------------------------------------------------
	class NightSkyBuilder
	{
	private:
 					NightSkyBuilder(	const NightSkyBuilder&);
 		NightSkyBuilder& operator=(		const NightSkyBuilder&);
	public:
					NightSkyBuilder(	int _resolution);
				   ~NightSkyBuilder();
		void		SetSunFactor(		float _factor);
		void		SetPosition(		float _theta,
										float _phi);
		void		SetTurbidity(		float _turbidity);
		void		Update(				bool _drawSun=true);

		GLuint							skyFramebuffer;
		TextureArray2D					skyTexture;
		Texture1D						starTexture;
		int								resolution;
		float							sunTheta;
		float							sunPhi;
		float 							sunFactor;
		glm::vec3						sunIntensity;
		float							turbidity;
		int								nStars;

		struct Sky
		{
										Sky():program("NightSky"){}
			GLint						sunSphCoordVar;
			GLint						turbidityVar;
			GLint						sunFactorVar;
			GLint						drawSunVar;
			Program						program;
		};

		struct Star
		{
										Star():program("NightStar"){}
			GLint						starTexUnit;
			GLint						starFactorVar;
			Program						program;
		};

		Sky								sky;
		Star							star;
		VertexBuffer3F					cubeVBO;
		VertexArray						cubeVAO;
		VertexBuffer3F					pointVBO;
		VertexArray						pointVAO;
	private:
		glm::vec3 	ToCartesian(		float _theta, 
										float _phi);
		float 		PerezFunction(		float A,  
										float B,  
										float C,  
										float D,  
										float E,  
										float cosTheta, 
										float gamma,
										float thetaS, 
										float lvz );
		glm::vec3 	ComputeSunIntensity(float _sunTheta, 
										float _sunPhi, 
										float _turbidity);
		void 		UpdateStar(			float _maxIntensity);
	};
	//--------------------------------------------------------------------------
}
#endif
