//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/sh.hpp>
#include <glf/utils.hpp>
#include <glm/gtx/transform.hpp>
#include <glf/window.hpp>
#include <glf/geometry.hpp>

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define DISPLAY_SH_COEFFICIENTS 0

namespace glf
{
	//--------------------------------------------------------------------------
	SHLight::SHLight() 
	{ 
		for(int i=0;i<9;++i)
			coeffs[i]=glm::vec3(0);
	}
	//--------------------------------------------------------------------------
	SHBuilder::SHBuilder(int _resolution):
	programProjection("SHProjection"),
	resolution(_resolution)
	{
		programProjection.Compile(	LoadFile("../resources/shaders/shbuilder.vs"),
									LoadFile("../resources/shaders/shbuilder.fs"));
		glm::mat4 transformation = ScreenQuadTransform();
		glProgramUniformMatrix4fv(programProjection.id, programProjection["Transformation"].location, 1, GL_FALSE, &transformation[0][0]);

		CreateQuad(vbo);
		vao.Add(vbo,semantic::Position,3,GL_FLOAT);

		glm::mat4 transformations[6];
		transformations[0] = glm::rotate(-90.f,0.f,0.f,1.f) * glm::rotate(90.f,1.f,0.f,0.f); 				// Positive X
		transformations[1] = glm::rotate( 90.f,0.f,0.f,1.f) * glm::rotate(90.f,1.f,0.f,0.f); 				// Negative X
		transformations[2] = glm::rotate( 90.f,1.f,0.f,0.f);												// Positive Y
		transformations[3] = glm::rotate(180.f,0.f,0.f,1.f) * glm::rotate(90.f,1.f,0.f,0.f); 				// Negative Y
		transformations[4] = glm::rotate(180.f,1.f,0.f,0.f);  												// Positive Z
		transformations[5] = glm::mat4(1);																	// Negative Z
		glProgramUniformMatrix4fv(programProjection.id, programProjection["Transformations"].location, 6, GL_FALSE, &transformations[0][0][0]);

		texProjectionUnit  = programProjection["CubeMap"].unit;
		glProgramUniform1i(programProjection.id, programProjection["CubeMap"].location, texProjectionUnit);

		// Would be 16FP enough precise ?
		shTexture.Allocate(GL_RGBA32F,_resolution,_resolution,7,true);
		shTexture.SetWrapping(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
		shTexture.SetFiltering(GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR);

		glGenFramebuffers(1,&shFrameProjection);
		glBindFramebuffer(GL_FRAMEBUFFER,shFrameProjection);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + programProjection.Output("SHCoeffs0"), shTexture.id, 0, 0);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + programProjection.Output("SHCoeffs1"), shTexture.id, 0, 1);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + programProjection.Output("SHCoeffs2"), shTexture.id, 0, 2);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + programProjection.Output("SHCoeffs3"), shTexture.id, 0, 3);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + programProjection.Output("SHCoeffs4"), shTexture.id, 0, 4);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + programProjection.Output("SHCoeffs5"), shTexture.id, 0, 5);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + programProjection.Output("SHCoeffs6"), shTexture.id, 0, 6);

		GLenum buffers[7] = {	GL_COLOR_ATTACHMENT0, 
								GL_COLOR_ATTACHMENT1,
								GL_COLOR_ATTACHMENT2,
								GL_COLOR_ATTACHMENT3,
								GL_COLOR_ATTACHMENT4,
								GL_COLOR_ATTACHMENT5,
								GL_COLOR_ATTACHMENT6 };
		glDrawBuffers(7,buffers);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		assert(glf::CheckFramebuffer(shFrameProjection));
		assert(glf::CheckError("SHBuilder::SHBuilder"));
	}
	//--------------------------------------------------------------------------
	void SHBuilder::Project(const TextureArray2D& 	_sourceTex, 
							SHLight& 				_light,
							int 					_level)
	{
		assert(_sourceTex.size.x == shTexture.size.x);
		assert(_sourceTex.size.y == shTexture.size.y);
		assert(_sourceTex.levels == shTexture.levels);

		_sourceTex.Bind(texProjectionUnit);

		// Face are ordered just as cubemap's face (+x,-x,+y,-y,+z,-z)
		glViewport(0,0,shTexture.size.x,shTexture.size.y);
		glBindFramebuffer(GL_FRAMEBUFFER,shFrameProjection);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(programProjection.id);
		vao.Draw(GL_TRIANGLES,6,0);
		assert(glf::CheckFramebuffer(shFrameProjection));

		// Reduce
		glBindTexture(GL_TEXTURE_2D_ARRAY,shTexture.id);
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

		// Retrieve coefficients
		// We multiple by the number of pixels in order to compensate the 
		// average process of the mipmap (we want the sum, not the average)
		//
		// The last two SH coeffs are stored into the first 6 w components
		int nMipmaps = glf::MipmapLevels(resolution);
		float factor = resolution*resolution;
		float coeffs[28];
		glGetTexImage(GL_TEXTURE_2D_ARRAY, nMipmaps-1, GL_RGBA, GL_FLOAT, coeffs);
		_light.coeffs[0] = glm::vec3(coeffs[0], coeffs[1], coeffs[2])  * factor;
		_light.coeffs[1] = glm::vec3(coeffs[4], coeffs[5], coeffs[6])  * factor;
		_light.coeffs[2] = glm::vec3(coeffs[8], coeffs[9], coeffs[10]) * factor;
		_light.coeffs[3] = glm::vec3(coeffs[12],coeffs[13],coeffs[14]) * factor;
		_light.coeffs[4] = glm::vec3(coeffs[16],coeffs[17],coeffs[18]) * factor;
		_light.coeffs[5] = glm::vec3(coeffs[20],coeffs[21],coeffs[22]) * factor;
		_light.coeffs[6] = glm::vec3(coeffs[24],coeffs[25],coeffs[26]) * factor;
		_light.coeffs[7] = glm::vec3(coeffs[3], coeffs[7], coeffs[11]) * factor;
		_light.coeffs[8] = glm::vec3(coeffs[15],coeffs[19],coeffs[23]) * factor;

		#if DISPLAY_SH_COEFFICIENTS
		for(int i=0;i<9;++i)
			glf::Info("Coeffs %d : %f %f %f",i,_light.coeffs[i].x,_light.coeffs[i].y,_light.coeffs[i].z);
		#endif

		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport(0,0,ctx::window.Size.x,ctx::window.Size.y);
		glf::CheckError("SHBuilder::Update");
	}
	//-------------------------------------------------------------------------
	SHRenderer::SHRenderer(int _w, int _h):
	program("SHRenderer")
	{
		program.Compile(LoadFile("../resources/shaders/shrenderer.vs"),
						LoadFile("../resources/shaders/shrenderer.fs"));

		shLightVar			= program["SHLight"].location;
		diffuseTexUnit		= program["DiffuseTex"].unit;
		normalTexUnit		= program["NormalTex"].unit;
		glm::mat4 trans		= ScreenQuadTransform();

		glProgramUniformMatrix4fv(program.id, program["Transformation"].location,	1, GL_FALSE, &trans[0][0]);
		glProgramUniform1i(program.id, 		  program["DiffuseTex"].location,		diffuseTexUnit);
		glProgramUniform1i(program.id, 		  program["NormalTex"].location,		normalTexUnit);
	}
	//-------------------------------------------------------------------------
	void SHRenderer::Draw(	const SHLight&	_light,
							const GBuffer&	_gbuffer,
							const RenderTarget& _renderTarget)
	{
		glUseProgram(program.id);

		glProgramUniform3fv(program.id,	shLightVar, 9, (float*)(&_light.coeffs[0]));
		_gbuffer.diffuseTex.Bind(diffuseTexUnit);
		_gbuffer.normalTex.Bind(normalTexUnit);
		_renderTarget.Draw();

		glf::CheckError("SHRenderer::Draw");
	}
}

