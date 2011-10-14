//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/light.hpp>
#include <glf/window.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace glf
{
	//-------------------------------------------------------------------------
	Light::Light(int _w, int _h):
	position(0,0,1),
	direction(0,0,-1),
	intensity(1.f),
	nearPlane(0.1f),
	farPlane(10.f)
	{
		glf::Info("Create light");
		depthTex.Allocate(GL_DEPTH_COMPONENT32F,_w,_h);
		depthTex.SetFiltering(GL_LINEAR,GL_LINEAR);
		depthTex.SetWrapping(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
		depthTex.SetCompare(GL_COMPARE_REF_TO_TEXTURE,GL_LEQUAL);

		// Initialize framebuffer
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT, depthTex.target, depthTex.id, 0);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glf::CheckFramebuffer(framebuffer);
		//glBindTexture(depthTex.target,0);

		assert(glf::CheckError("Light"));
	}
	//-------------------------------------------------------------------------
	Light::~Light()
	{
		glDeleteFramebuffers(1,&framebuffer);
	}
	//-------------------------------------------------------------------------
	void Light::SetPosition(		const glm::vec3& _position, 
									const glm::vec3& _direction,
									const BBox&		 _worldBound)
	{
		position		 = _position;
		direction		 = _direction;

		// Transform world bbox into a bsphere
		float radius	 = glm::length(_worldBound.pMax - _worldBound.pMin) * 0.5f;
		glm::vec3 center = (_worldBound.pMax + _worldBound.pMin)*0.5f;

		// Deduce near and far plane
		nearPlane = std::max(0.1f,glm::length(center-position)-radius);
		farPlane  = nearPlane + 2.f*radius;

		// Update view & proj matrices
		glm::vec3 up	= glm::vec3(0,0,1);
		glm::vec3 right = glm::normalize(glm::cross(direction,up));
		up				= glm::normalize(glm::cross(right,direction));
		view			= glm::lookAt(_position,_position+_direction,up);
//		proj			= glm::ortho(-0.5f*radius,0.5f*radius,-0.5f*radius,0.5f*radius);
		proj			= glm::perspective(60.f,depthTex.size.x/float(depthTex.size.y),nearPlane,farPlane);

		// Store a pre-composed matrix which take into account the projective to texture transformation [-1..1] -> [0..1]
		//transformation  = glm::translate(0.5f,0.5f,0.5f) * glm::scale(0.5f,0.5f,0.5f) * proj * view;
		transformation  = proj * view;
	}
	//-------------------------------------------------------------------------
	Light::Ptr Light::Create(int _w,int _h)
	{
		return Ptr(new Light(_w,_h));
	}
	//-------------------------------------------------------------------------
	ShadowRender::ShadowRender():
	program("ShadowRender")
	{
		program.Compile(LoadFile("../resources/shaders/light.vs"),
						LoadFile("../resources/shaders/light.fs"));

		projVar 		= program["Projection"].location;
		viewVar 		= program["View"].location;
		modelVar 		= program["Model"].location;
	}
	//-------------------------------------------------------------------------
	void ShadowRender::Draw(	const Light& _light,
								const std::vector<Object::Ptr>& _objects)
	{
		glCullFace(GL_FRONT);

		glUseProgram(program.id);
		glProgramUniformMatrix4fv(program.id, projVar,  1, GL_FALSE, &_light.proj[0][0]);
		glProgramUniformMatrix4fv(program.id, viewVar,  1, GL_FALSE, &_light.view[0][0]);

		glViewport(0,0,_light.depthTex.size.x,_light.depthTex.size.y);
		glBindFramebuffer(GL_FRAMEBUFFER,_light.framebuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
		for(unsigned int i=0;i<_objects.size();++i)
		{
			glProgramUniformMatrix4fv(program.id, modelVar, 1, GL_FALSE, &_objects[i]->model[0][0]);
			_objects[i]->Draw();
		}
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport(0,0,ctx::window.Size.x,ctx::window.Size.y);

		glCullFace(GL_BACK);
	}
	//-------------------------------------------------------------------------
	ShadowRender::Ptr ShadowRender::Create()
	{
		return ShadowRender::Ptr(new ShadowRender());
	}
	//-------------------------------------------------------------------------
	ShadowPass::ShadowPass(int _w, int _h):
	program("ShadowPass")
	{
		program.Compile(LoadFile("../resources/shaders/hardshadow.vs"),
						LoadFile("../resources/shaders/hardshadow.fs"));

		lightPosVar 		= program["LightPos"].location;
		lightTransfoVar		= program["LightTransfo"].location;
		lightIntensityVar	= program["LightIntensity"].location;

		vbufferVar			= program["Position"].location;
		positionTexUnit		= program["PositionTex"].unit;
		diffuseTexUnit		= program["DiffuseTex"].unit;
		normalTexUnit		= program["NormalTex"].unit;
		shadowTexUnit		= program["ShadowTex"].unit;

		glm::mat4 proj 		= glm::ortho(-0.5f,0.5f,-0.5f,0.5f,0.1f,100.f);
		glm::mat4 view 		= glm::lookAt(glm::vec3(0.5f,0.5f,5.0f),glm::vec3(0.5f,0.5f,-1.0f),glm::vec3(0.0f,1.0f,0.0f));

		glProgramUniformMatrix4fv(program.id, program["Projection"].location,	1, GL_FALSE, &proj[0][0]);
		glProgramUniformMatrix4fv(program.id, program["View"].location,			1, GL_FALSE, &view[0][0]);
		glProgramUniform1i(program.id, 		  program["PositionTex"].location,	positionTexUnit);
		glProgramUniform1i(program.id, 		  program["ShadowTex"].location,	shadowTexUnit);
		glProgramUniform1i(program.id, 		  program["DiffuseTex"].location,	diffuseTexUnit);
		glProgramUniform1i(program.id, 		  program["NormalTex"].location,	normalTexUnit);

		vbuffer.Resize(6);
		glm::vec3* vertices = vbuffer.Lock();
		vertices[0] = glm::vec3(0,0,0);
		vertices[1] = glm::vec3(1,0,0);
		vertices[2] = glm::vec3(1,1,0);
		vertices[3] = glm::vec3(0,0,0);
		vertices[4] = glm::vec3(1,1,0);
		vertices[5] = glm::vec3(0,1,0);
		vbuffer.Unlock();

		// Create output texture
		texture.Allocate(GL_RGB32F,_w,_h);

		// Create framebuffer
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, texture.target, texture.id, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glf::CheckFramebuffer(framebuffer);
		glf::CheckError("ShadowPass::Create");
	}
	//-------------------------------------------------------------------------
	void ShadowPass::Draw(	const Light&	_light,
							const GBuffer&	_gbuffer)
	{
		glUseProgram(program.id);

		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glProgramUniformMatrix4fv(program.id,	lightTransfoVar,	1, GL_FALSE, &_light.transformation[0][0]);
		glProgramUniform3f(program.id,			lightPosVar,		_light.position.x, _light.position.y, _light.position.z);
		glProgramUniform1f(program.id,			lightIntensityVar,	_light.intensity);

		_light.depthTex.Bind(shadowTexUnit);
		_gbuffer.positionTex.Bind(positionTexUnit);
		_gbuffer.diffuseTex.Bind(diffuseTexUnit);
		_gbuffer.normalTex.Bind(normalTexUnit);

		glBindBuffer(GL_ARRAY_BUFFER, vbuffer.id);
		glVertexAttribPointer(	vbufferVar, 
								3, 
								GL_FLOAT, 
								false,
								sizeof(glm::vec3),
								GLF_BUFFER_OFFSET(0));
		glEnableVertexAttribArray(vbufferVar);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER,0);

		glf::CheckError("LightPass3");
	}
	//-------------------------------------------------------------------------
	ShadowPass::Ptr ShadowPass::Create(int _w, int _h)
	{
		return ShadowPass::Ptr(new ShadowPass(_w,_h));
	}
}
