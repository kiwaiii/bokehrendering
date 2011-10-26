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
		void 		Draw(				const Texture2D& _colorTex, 
										const Texture2D& _positionTex, 
										const glm::mat4& _view,
										float 			_nearStart,
										float 			_nearEnd,
										float 			_farStart,
										float 			_farEnd,
										float 			_maxCoCRadius,
										float 			_maxBokehRadius,
										int 			_nSamples,
										float 			_intensityThreshold,
										float 			_cocThreshold,
										float			_bokehDepthCutoff,
										const RenderTarget& _target);
	public:
		//----------------------------------------------------------------------
		struct ResetPass
		{

		};
		//----------------------------------------------------------------------
		struct CoCPass
		{
										CoCPass():program("DOF::CoCPass"){}
			GLint 						positionTexUnit;
			GLint						farStartVar;		// Far start
			GLint						farEndVar;			// Far end
			GLint						viewMatVar;			// View matrix

			Program 					program;
		};
		//----------------------------------------------------------------------
		struct DetectionPass
		{
										DetectionPass():program("DOF::DetectionPass"){}
			GLint 						colorTexUnit;
			GLint 						blurDepthTexUnit;
			GLint 						cocThresholdVar;
			GLint 						lumThresholdVar;
			GLint 						maxCoCRadiusVar;
			GLint 						bokehCountTexUnit;
			GLint 						bokehColorTexUnit;
			GLint 						bokehPositionTexUnit;

			Program 					program;
		};
		//----------------------------------------------------------------------
		struct BlurPass
		{
										BlurPass():program("DOF::BlurPass"){}
			GLint 						colorTexUnit;
			GLint						blurDepthTexUnit;
			GLint						directionVar;
			GLint						maxCoCRadiusVar;

			Program 					program;
		};
		//----------------------------------------------------------------------
		struct RenderingPass
		{
										RenderingPass():program("DOF::RenderingPass"){}
			GLint 						bokehPositionTexUnit;
			GLint 						bokehColorTexUnit;
			GLint 						bokehShapeTexUnit;
			GLint 						blurDepthTexUnit;
			GLint						maxBokehRadiusVar;
			GLint						bokehDepthCutoffVar;

			Program 					program;
		};

//	private:
		Texture2D						blurDepthTex;
		Texture2D						detectionTex;
		Texture2D						blurTex;
		Texture2D						bokehShapeTex;

		Texture2D						bokehPositionTex;	// Store bokeh position
		Texture2D						bokehColorTex;		// Store bokeh color
		GLuint							bokehCountTexID;	// Atomic bokeh counter

		GLuint							blurDepthFBO;
		GLuint							detectionFBO;
		GLuint							blurFBO;

		CoCPass 						cocPass;
		DetectionPass					detectionPass;
		BlurPass						blurPass;
		RenderingPass					renderingPass;
		
		VertexBuffer3F					pointVBO;
		VertexArray						pointVAO;
		IndirectArrayBuffer				pointIndirectBuffer;
	};
}

#endif
