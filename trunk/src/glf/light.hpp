#ifndef GLF_LIGHT_HPP
#define GLF_LIGHT_HPP

//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glm/glm.hpp>
#include <glf/bound.hpp>
#include <glf/wrapper.hpp>
#include <glf/spointer.hpp>	
#include <glf/scenegraph.hpp>	
#include <glf/pass.hpp>
#include <vector>

namespace glf
{
	class Light
	{
	public:
		typedef		glf::SmartPointer<Light> Ptr;

				   ~Light();
		void		SetIntensity(	float _intensity);
		void		SetPosition(	const glm::vec3& _position, 
									const glm::vec3& _direction,
									const BBox& _worldBound);
		static Ptr	Create(			int _w,
									int _h);
	private:
					Light(			int _w, 
									int _h);
 					Light(			const Light&);
 		Light		operator=(		const Light&);
	public:
		//---------------------------------------------------------------------
		// Attributes
		//---------------------------------------------------------------------
		glm::mat4	proj;
		glm::mat4	view;
		glm::mat4	transformation;
		glm::vec3 	position;
		glm::vec3 	direction;
		float 		intensity;
		float		size;
		float 		nearPlane;
		float 		farPlane;
		Texture2D	depthTex;
		GLuint		framebuffer;
	};
	
	class ShadowRender
	{
	public:
		typedef		glf::SmartPointer<ShadowRender> Ptr;
		static Ptr	Create();
		void 		Draw(			const Light& _light,
									const std::vector<Object::Ptr>& _objects);
	private:
					ShadowRender();
 					ShadowRender(	const ShadowRender&);
 		ShadowRender operator=(		const ShadowRender&);
	public:
		//---------------------------------------------------------------------
		// Attributes
		//---------------------------------------------------------------------
		GLint 		projVar;
		GLint 		viewVar;
		GLint 		modelVar;
		Program 	program;
	};

	class ShadowPass
	{
	public:
		typedef		glf::SmartPointer<ShadowPass> Ptr;
		static Ptr	Create(int _w, int _h);
		void 		Draw(			const Light&	_light,
									const GBuffer&	_gbuffer);
		//todo : add a draw on light ?
	private:
					ShadowPass(int _w, int _h);
 					ShadowPass(		const ShadowPass&);
 		ShadowPass	operator=(		const ShadowPass&);
	public:
		//---------------------------------------------------------------------
		// Attributes
		//---------------------------------------------------------------------
		GLint 							positionTexUnit;
		GLint 							diffuseTexUnit;
		GLint 							normalTexUnit;
		GLint 							shadowTexUnit;
		

		GLint 							lightTransfoVar;
		GLint							lightPosVar;
		GLint							lightIntensityVar;

		GLint 							vbufferVar;
		GLuint							framebuffer;
		glm::mat4 						proj;
		glm::mat4 						view;
		Program 						program;
		Texture2D						texture;
		VertexBuffer<glm::vec3>::Buffer vbuffer;
	};
}
#endif
