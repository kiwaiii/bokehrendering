#ifndef GLF_DOF_HPP
#define GLF_DOF_HPP

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/wrapper.hpp>
#include <glf/texture.hpp>

namespace glf
{
	//--------------------------------------------------------------------------
	class DOFProcessor
	{
	public:
		typedef		glf::SmartPointer<DOFProcessor> Ptr;
		static Ptr	Create(				int _w, 
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
										float			_areaFactor);
	private:
					DOFProcessor(		int _w, 
										int _h);
 					DOFProcessor(		const DOFProcessor&);
 		DOFProcessor operator=(			const DOFProcessor&);
	public:
		//----------------------------------------------------------------------
		struct CoCPass
		{
										CoCPass():program("CoCPass"){}
			GLint 						positionTexUnit;
			GLint 						rotationTexUnit;
			GLint 						inputTexUnit;
			GLint 						sampleTexUnit;
			GLint 						colorTexUnit;
			GLint 						countTexUnit;

			GLint						nearStartVar;
			GLint						nearEndVar;
			GLint						farStartVar;
			GLint						farEndVar;
			GLint						maxRadiusVar;
			GLint						viewMatVar;
			GLint						nSamplesVar;
			GLint						intThresholdVar;
			GLint						cocThresholdVar;
			GLint						areaFactorVar;

			Program 					program;
			Texture2D					rotationTex;
		};
		//----------------------------------------------------------------------
		struct BokehPass
		{	
										BokehPass():program("BokehPass"){}
			GLint 						sampleTexUnit;
			GLint 						bokehTexUnit;
			GLint 						colorTexUnit;
			GLint						attenuationVar;

			Program 					program;
			Texture2D					bokehTex;
		};
		//----------------------------------------------------------------------
		int								maxBokehCount;
		GLuint 							framebuffer;
		Texture2D						composeTex;
		Texture2D						sampleTex;
		Texture2D						colorTex;
		GLuint							countTexID;

		GLint 							vbufferVar;
		VertexBuffer<glm::vec3>::Buffer quadbuffer;
		VertexBuffer<glm::vec3>::Buffer pointbuffer;
		IndirectArrayBuffer::Buffer		indirectBuffer;

		CoCPass 						cocPass;
		BokehPass 						bokehPass;
	};
}

#endif
