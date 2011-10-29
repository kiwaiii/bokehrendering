//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/postprocessor.hpp>
#include <glf/window.hpp>
#include <glf/utils.hpp>
#include <glf/geometry.hpp>


namespace glf
{
	//--------------------------------------------------------------------------
	PostProcessor::ToneMapping::ToneMapping():
	program("ToneMapping")
	{
	
	}
	//--------------------------------------------------------------------------
	PostProcessor::PostProcessor(		unsigned int _width, 
										unsigned int _height)
	{
		glm::mat4 transform = ScreenQuadTransform();

		// Tone Mapping
		{
			toneMapping.program.Compile(	LoadFile(directory::ShaderDirectory + "tonemap.vs"),
											LoadFile(directory::ShaderDirectory + "tonemap.fs") );
			toneMapping.colorTexUnit		= toneMapping.program["ColorTex"].unit;
			toneMapping.exposureVar			= toneMapping.program["Exposure"].location;
			glProgramUniform1i(toneMapping.program.id, toneMapping.program["ColorTex"].location, toneMapping.colorTexUnit);
			glProgramUniformMatrix4fv(toneMapping.program.id, toneMapping.program["Transformation"].location,1, GL_FALSE, &transform[0][0]);
		}
	}
	//--------------------------------------------------------------------------
	PostProcessor::~PostProcessor()
	{
	}
	//--------------------------------------------------------------------------
	void PostProcessor::Draw(			const Texture2D& _colorTex,
										float _toneExposure,
										const RenderTarget& _renderTarget)
	{
		glUseProgram(toneMapping.program.id);
		glProgramUniform1f(toneMapping.program.id, toneMapping.exposureVar, _toneExposure);
		_colorTex.Bind(toneMapping.colorTexUnit);
		_renderTarget.Draw();
	}
	//--------------------------------------------------------------------------
}

