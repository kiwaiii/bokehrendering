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
		CreateQuad(vbo);
		vao.Add(vbo,semantic::Position,3,GL_FLOAT);
		glm::mat4 transform = ScreenQuadTransform();

		// Tone Mapping
		toneMapping.program.Compile(	LoadFile("../resources/shaders/tonemap.vs"),
										LoadFile("../resources/shaders/tonemap.fs") );
		toneMapping.inputTexUnit		= toneMapping.program["InputTex"].unit;
		toneMapping.exposureVar			= toneMapping.program["Exposure"].location;
		glProgramUniform1i(toneMapping.program.id, toneMapping.program["InputTex"].location, toneMapping.inputTexUnit);
		glProgramUniformMatrix4fv(toneMapping.program.id, toneMapping.program["Transformation"].location,1, GL_FALSE, &transform[0][0]);
	}
	//--------------------------------------------------------------------------
	PostProcessor::~PostProcessor()
	{
	}
	//--------------------------------------------------------------------------
	void PostProcessor::Apply(			const Texture2D& _inputTex,
										float _toneExposure)
	{
		glUseProgram(toneMapping.program.id);
		glProgramUniform1f(toneMapping.program.id, toneMapping.exposureVar, _toneExposure);
		_inputTex.Bind(toneMapping.inputTexUnit);
		vao.Draw(GL_TRIANGLES,vbo.count,0);
	}
	//--------------------------------------------------------------------------
}

