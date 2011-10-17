#ifndef GLF_DOF_HPP
#define GLF_DOF_HPP

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/wrapper.hpp>
#include <glf/texture.hpp>
#include <glf/pass.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	class DOFProcessor
	{
	private:
 					DOFProcessor(		const DOFProcessor&);
 		DOFProcessor operator=(			const DOFProcessor&);
	public:
					DOFProcessor(		int _w, 
										int _h);
		void 		Draw(				const Texture2D& _inputTex, 
										const Texture2D& _positionTex, 
										const glm::mat4& _view,
										float 			_nearStart,
										float 			_nearEnd,
										float 			_farStart,
										float 			_farEnd,
										float 			_maxRadius,
										int 			_nSamples,
										float 			_intensityThreshold,
										float 			_cocThreshold,
										float			_attenuation,
										float			_areaFactor,
										const RenderTarget& _target);
	public:
		//----------------------------------------------------------------------
		struct CoCPass
		{
										CoCPass():program("CoCPass"){}
			GLint 						positionTexUnit;
			GLint 						rotationTexUnit;
			GLint 						inputTexUnit;
			GLint 						bokehPosTexUnit;
			GLint 						bokehColorTexUnit;
			GLint 						bokehCountTexUnit;

			GLint						nearStartVar;		// Near start
			GLint						nearEndVar;			// Near end
			GLint						farStartVar;		// Far start
			GLint						farEndVar;			// Far end
			GLint						maxRadiusVar;		// Max CoC radius
			GLint						viewMatVar;			// View matrix
			GLint						nSamplesVar;		// #samples for computing blur
			GLint						intThresholdVar;	// ?
			GLint						cocThresholdVar;	// CoC threshold ?
			GLint						areaFactorVar;		// Weight of the bokeh integral

			Program 					program;
			Texture2D					rotationTex;
		};
		//----------------------------------------------------------------------
		struct BokehPass
		{	
										BokehPass():program("BokehPass"){}
			GLint 						bokehPosTexUnit;
			GLint 						bokehShapeTexUnit;
			GLint 						bokehColorTexUnit;
			GLint						attenuationVar;

			Program 					program;
			Texture2D					bokehShapeTex;
		};
	private:
		//----------------------------------------------------------------------
		int								maxBokehCount;
		Texture2D						bokehPosTex;		// Store bokeh position
		Texture2D						bokehColorTex;		// Store bokeh color
		GLuint							bokehCountTexID;

		VertexBuffer3F					pointVBO;
		VertexArray						pointVAO;
		IndirectArrayBuffer				pointIndirectBuffer;

		CoCPass 						cocPass;
		BokehPass 						bokehPass;
	};
}

#endif
