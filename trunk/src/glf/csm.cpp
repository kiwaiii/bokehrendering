//-----------------------------------------------------------------------------
// Include
//-----------------------------------------------------------------------------
#include <glf/csm.hpp>
#include <glm/gtx/transform.hpp>
#include <glf/window.hpp>

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define ALIGN_CSM_WITH_CAMERA	1
#define ENABLE_HELPERS			0

namespace glf
{
	//-------------------------------------------------------------------------
	// Corner0 : -1 -1 
	// Corner1 :  1 -1 
	// Corner2 :  1  1 
	// Corner3 : -1  1 
	//-------------------------------------------------------------------------
	void FrustumPlaneExtraction
	(
		const glm::vec3& _camPos,
		const glm::vec3& _camDir,
		const glm::vec3& _camUp,
		float _camRatio,
		float _camFov,
		float _camZ,
		glm::vec3& _corner0,
		glm::vec3& _corner1,
		glm::vec3& _corner2,
		glm::vec3& _corner3
	)
	{
		// Compute half size of near and far planes
		float hHeight  	 = tan(_camFov*0.5f) * _camZ;
		float hWidth   	 = hHeight * _camRatio;
		glm::vec3 center = _camPos + _camDir * _camZ;
		glm::vec3 right	 = glm::normalize(glm::cross(_camDir,_camUp));

		// Deduce bounding points
		_corner0 = center - (_camUp * hHeight) - (right * hWidth);
		_corner1 = center - (_camUp * hHeight) + (right * hWidth);
		_corner2 = center + (_camUp * hHeight) + (right * hWidth);
		_corner3 = center + (_camUp * hHeight) - (right * hWidth);
	}
	//-------------------------------------------------------------------------
	CSMLight::CSMLight(int _w, int _h,int _nCascades):
	direction(0,0,-1),
	intensity(1.f,1.f,1.f),
	nCascades(_nCascades)
	{
		glf::Info("CSMLight::CSMLight");
		assert(nCascades<=4);

		projs 		= new glm::mat4[nCascades];
		viewprojs	= new glm::mat4[nCascades];
		nearPlanes	= new float[nCascades];
		farPlanes	= new float[nCascades];

		depthTexs.Allocate(GL_DEPTH_COMPONENT32F,_w,_h,nCascades);
		depthTexs.SetFiltering(GL_LINEAR,GL_LINEAR);
		depthTexs.SetWrapping(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
		depthTexs.SetCompare(GL_COMPARE_REF_TO_TEXTURE,GL_LEQUAL);

		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexs.id, 0);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glf::CheckFramebuffer(framebuffer);

		// Default init
		for(int i=0;i<nCascades;++i)
		{
			nearPlanes[i] = 0.1f;
			farPlanes[i]  = 100.f;
		}		
		assert(glf::CheckError("CSMLight::CSMLight"));
	}
	//-------------------------------------------------------------------------
	CSMLight::~CSMLight()
	{
		delete[] farPlanes;
		delete[] nearPlanes;
		delete[] viewprojs;
		delete[] projs;
		glDeleteFramebuffers(1,&framebuffer);
	}
	//-------------------------------------------------------------------------
	void CSMLight::SetDirection(	const glm::vec3& _direction)
	{
		direction		 = _direction;
	}
	//-------------------------------------------------------------------------
	void CSMLight::SetIntensity(	const glm::vec3& _intensity)
	{
		intensity = _intensity;
	}
	//-------------------------------------------------------------------------
	CSMBuilder::CSMBuilder():
	program("CSMBuilder")
	{
		program.Compile(LoadFile("../resources/shaders/csmbuilder.vs"),
						LoadFile("../resources/shaders/csmbuilder.gs"),
						LoadFile("../resources/shaders/csmbuilder.fs"));

		projVar 		= program["Projections"].location;
		viewVar 		= program["View"].location;
		modelVar 		= program["Model"].location;
		nCascadesVar	= program["nCascades"].location;
	}
	//-------------------------------------------------------------------------
	void CSMBuilder::Draw(	CSMLight&			_light,
							const Camera&		_camera,
							float 				_cascadeAlpha,
							float 				_blendFactor,
							const SceneManager& _scene,
							HelperManager& 		_helpers)
	{
		_helpers.Clear();

		// Extract camera near/far
		float n = _camera.Near();
		float f = _camera.Far();

		// Extract near plane corners
		glm::mat4 camView	= _camera.View();
		glm::vec3 camPos	= _camera.Eye();
		glm::vec3 camDir	=  glm::normalize(_camera.Center()-camPos);
		glm::vec3 camUp		= _camera.Up();
		glm::vec3 camRight	=  glm::normalize(glm::cross(camDir,camUp));
		float camFov		= _camera.Fov();
		float camRatio		= _camera.Ratio();

		#if ENABLE_HELPERS
		_helpers.CreateReferential(camRight,camUp,-camDir,0.3f,glm::translate(camPos.x,camPos.y,camPos.z));
		#endif

		// Compute scene AABB in view space
		// Update according to the scene bound
		// Invert min.z/max.z bounds because of the view matrix : objects are into the negative z part
		BBox sceneView   	= Transform(_scene.wBound,camView);
		n 					= std::max(n,-sceneView.pMax.z);
		f 					= std::min(f,-sceneView.pMin.z);

		// Compute lightView matrix (CSM is aligned with camera)
		glm::vec3 lightTar	= camPos;
		glm::vec3 lightDir	= _light.direction;
		glm::vec3 lightUp	= -camRight;
		if(fabs(glm::dot(lightUp,lightDir))>0.9f) lightUp = camUp;
		glm::vec3 lightRight= glm::normalize(glm::cross(lightDir,lightUp));
		lightUp				= glm::normalize(glm::cross(lightRight,lightDir));
		_light.view			= glm::lookAt(lightTar,lightTar+lightDir,lightUp);
		_light.camView		= camView;

		#if ENABLE_HELPERS
		_helpers.CreateReferential(lightRight,lightUp,-lightDir,1.f,glm::translate(camPos.x,camPos.y,camPos.z));
		#endif

		// Compute scene AABB in light space
		BBox sceneLight   	= Transform(_scene.wBound,_light.view);

		// Project corners into light space
		// TODO : Change to a direct transform from cam project space to light space instead of world to light space ?
		glm::vec3 c0,c1,c2,c3;
		FrustumPlaneExtraction(camPos,camDir,camUp,camRatio,camFov,n,c0,c1,c2,c3);
		glm::vec4 c00_v 	= _light.view * glm::vec4(c0,1.f);
		glm::vec4 c01_v 	= _light.view * glm::vec4(c1,1.f);
		glm::vec4 c02_v 	= _light.view * glm::vec4(c2,1.f);
		glm::vec4 c03_v 	= _light.view * glm::vec4(c3,1.f);

		#if ENABLE_HELPERS
		_helpers.CreateBound(c0,c1,c2,c3);
		#endif

		// For each cascade
		float previousFar	= n;
		for(int i=0;i<_light.nCascades;++i)
		{
			// Divide frustum with a magic formula 
			// (see : http://software.intel.com/en-us/articles/shadowexplorer/)
			float w			= ( i + 1.f ) / float(_light.nCascades);
			float camSpaceZ = _cascadeAlpha * n * pow( f / n, w ) + ( 1 - _cascadeAlpha ) * ( n + w * ( f - n ) );

			// Compute the far split plane
			FrustumPlaneExtraction(camPos,camDir,camUp,camRatio,camFov,camSpaceZ,c0,c1,c2,c3);
			glm::vec4 c10_v = _light.view * glm::vec4(c0,1.f);
			glm::vec4 c11_v = _light.view * glm::vec4(c1,1.f);
			glm::vec4 c12_v = _light.view * glm::vec4(c2,1.f);
			glm::vec4 c13_v = _light.view * glm::vec4(c3,1.f);

			#if ENABLE_HELPERS
			glm::mat4 viewInverse = glm::inverse(_light.view);
			_helpers.CreateBound(	glm::vec3(viewInverse * c00_v),
									glm::vec3(viewInverse * c01_v),
									glm::vec3(viewInverse * c02_v),
									glm::vec3(viewInverse * c03_v),

									glm::vec3(viewInverse * c10_v),
									glm::vec3(viewInverse * c11_v),
									glm::vec3(viewInverse * c12_v),
									glm::vec3(viewInverse * c13_v),

									glm::mat4(1.f),
									glm::vec3(1,0,1));
			#endif

			// Compute AABB in light space
			BBox boundSplit;
			boundSplit.Add(glm::vec3(c00_v));
			boundSplit.Add(glm::vec3(c01_v));
			boundSplit.Add(glm::vec3(c02_v));
			boundSplit.Add(glm::vec3(c03_v));
			boundSplit.Add(glm::vec3(c10_v));
			boundSplit.Add(glm::vec3(c11_v));
			boundSplit.Add(glm::vec3(c12_v));
			boundSplit.Add(glm::vec3(c13_v));

			// Extract min-max Z-range in light space (take in accound scene bounds)
			boundSplit.pMin.x = glm::max(sceneLight.pMin.x, boundSplit.pMin.x);	
			boundSplit.pMax.x = glm::min(sceneLight.pMax.x, boundSplit.pMax.x);
			boundSplit.pMin.y = glm::max(sceneLight.pMin.y, boundSplit.pMin.y);	
			boundSplit.pMax.y = glm::min(sceneLight.pMax.y, boundSplit.pMax.y);
			boundSplit.pMin.z = sceneLight.pMin.z;
			boundSplit.pMax.z = sceneLight.pMax.z;

			// Save the far split plane for the next split (it becomes the near split plane)
			c00_v = c10_v;
			c01_v = c11_v;
			c02_v = c12_v;
			c03_v = c13_v;

			// Copy the camera z of the current split
			_light.nearPlanes[i]= previousFar;
			_light.farPlanes[i]	= camSpaceZ;
			previousFar			= camSpaceZ;
			//glf::Info("CamZ : %f",camSpaceZ);

			// Compute the light projection based on split AABB
			// Inverse z because of the light view matrix which has negative z
			_light.projs[i]		= glm::ortho(boundSplit.pMin.x,  boundSplit.pMax.x,
											 boundSplit.pMin.y,  boundSplit.pMax.y,
											-boundSplit.pMax.z, -boundSplit.pMin.z);
			_light.viewprojs[i]	= _light.projs[i] * _light.view;

			#if ENABLE_HELPERS
			glm::mat4 invViewProj = glm::inverse(_light.viewprojs[i]);
			_helpers.CreateBound(	glm::vec3(invViewProj * glm::vec4(-1,-1,-1, 1)),
									glm::vec3(invViewProj * glm::vec4( 1,-1,-1, 1)),
									glm::vec3(invViewProj * glm::vec4( 1, 1,-1, 1)),
									glm::vec3(invViewProj * glm::vec4(-1, 1,-1, 1)),

									glm::vec3(invViewProj * glm::vec4(-1,-1, 1, 1)),
									glm::vec3(invViewProj * glm::vec4( 1,-1, 1, 1)),
									glm::vec3(invViewProj * glm::vec4( 1, 1, 1, 1)),
									glm::vec3(invViewProj * glm::vec4(-1, 1, 1, 1)));
			#endif
		}

		// Render cascaded shadow maps 
		assert(_light.nCascades<=4);
		glUseProgram(program.id);
		glProgramUniform1i(program.id, 			nCascadesVar,	_light.nCascades);
		glProgramUniformMatrix4fv(program.id, 	projVar,  		_light.nCascades, 	GL_FALSE, &_light.projs[0][0][0]);
		glProgramUniformMatrix4fv(program.id, 	viewVar,  		1, 					GL_FALSE, &_light.view[0][0]);

		glViewport(0,0,_light.depthTexs.size.x,_light.depthTexs.size.y);
		glBindFramebuffer(GL_FRAMEBUFFER,_light.framebuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
		for(unsigned int o=0;o<_scene.shadowMeshes.size();++o)
		{
			glProgramUniformMatrix4fv(program.id, modelVar, 1, GL_FALSE, &_scene.transformations[o][0][0]);
			_scene.shadowMeshes[o].Draw();
		}
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport(0,0,ctx::window.Size.x,ctx::window.Size.y);
		glf::CheckError("CSMBuilder::Draw");
	}
	//-------------------------------------------------------------------------
	CSMRenderer::CSMRenderer(int _w, int _h):
	program("CSMRenderer")
	{
		program.Compile(LoadFile("../resources/shaders/csmrenderer.vs"),
						LoadFile("../resources/shaders/csmrenderer.fs"));

		lightDirVar 		= program["LightDir"].location;
		lightViewProjsVar	= program["LightViewProjs"].location;
		lightIntensityVar	= program["LightIntensity"].location;
		biasVar				= program["Bias"].location;
		apertureVar			= program["Aperture"].location;
		nSamplesVar			= program["nSamples"].location;
		nCascadesVar		= program["nCascades"].location;

		positionTexUnit		= program["PositionTex"].unit;
		diffuseTexUnit		= program["DiffuseTex"].unit;
		normalTexUnit		= program["NormalTex"].unit;
		shadowTexUnit		= program["ShadowTex"].unit;

		glm::mat4 transform = ScreenQuadTransform();

		// PCF samples
		// Halton sequence generated using: WONG, T.-T., LUK, W.-S., AND HENG, 
		// P.-A. 1997.Sampling with hammersley and Halton points
		// http://www.cse.cuhk.edu.hk/~ttwong/papers/udpoint/udpoint.pdf
		glm::vec2 haltonPoints[32];
		haltonPoints[0]  = glm::vec2(-0.353553, 0.612372);
		haltonPoints[1]  = glm::vec2(-0.25, -0.433013);
		haltonPoints[2]  = glm::vec2(0.663414, 0.55667);
		haltonPoints[3]  = glm::vec2(-0.332232, 0.120922);
		haltonPoints[4]  = glm::vec2(0.137281, -0.778559);
		haltonPoints[5]  = glm::vec2(0.106337, 0.603069);
		haltonPoints[6]  = glm::vec2(-0.879002, -0.319931);
		haltonPoints[7]  = glm::vec2(0.191511, -0.160697);
		haltonPoints[8]  = glm::vec2(0.729784, 0.172962);
		haltonPoints[9]  = glm::vec2(-0.383621, 0.406614);
		haltonPoints[10] = glm::vec2(-0.258521, -0.86352);
		haltonPoints[11] = glm::vec2(0.258577, 0.34733);
		haltonPoints[12] = glm::vec2(-0.82355, 0.0962588);
		haltonPoints[13] = glm::vec2(0.261982, -0.607343);
		haltonPoints[14] = glm::vec2(-0.0562987, 0.966608);
		haltonPoints[15] = glm::vec2(-0.147695, -0.0971404);
		haltonPoints[16] = glm::vec2(0.651341, -0.327115);
		haltonPoints[17] = glm::vec2(0.47392, 0.238012);
		haltonPoints[18] = glm::vec2(-0.738474, 0.485702);
		haltonPoints[19] = glm::vec2(-0.0229837, -0.394616);
		haltonPoints[20] = glm::vec2(0.320861, 0.74384);
		haltonPoints[21] = glm::vec2(-0.633068, -0.0739953);
		haltonPoints[22] = glm::vec2(0.568478, -0.763598);
		haltonPoints[23] = glm::vec2(-0.0878153, 0.293323);
		haltonPoints[24] = glm::vec2(-0.528785, -0.560479);
		haltonPoints[25] = glm::vec2(0.570498, -0.13521);
		haltonPoints[26] = glm::vec2(0.915797, 0.0711813);
		haltonPoints[27] = glm::vec2(-0.264538, 0.385706);
		haltonPoints[28] = glm::vec2(-0.365725, -0.76485);
		haltonPoints[29] = glm::vec2(0.488794, 0.479406);
		haltonPoints[30] = glm::vec2(-0.948199, 0.263949);
		haltonPoints[31] = glm::vec2(0.0311802, -0.121049);

		glProgramUniformMatrix4fv(program.id, program["Transformation"].location,	1, GL_FALSE, &transform[0][0]);
		glProgramUniform1i(program.id, 		  program["PositionTex"].location,		positionTexUnit);
		glProgramUniform1i(program.id, 		  program["ShadowTex"].location,		shadowTexUnit);
		glProgramUniform1i(program.id, 		  program["DiffuseTex"].location,		diffuseTexUnit);
		glProgramUniform1i(program.id, 		  program["NormalTex"].location,		normalTexUnit);
		glProgramUniform2fv(program.id,		  program["HaltonPoints"].location,		32, (float*)haltonPoints);

		glf::CheckError("CSMRenderer::Create");
	}
	//-------------------------------------------------------------------------
	void CSMRenderer::Draw(	const CSMLight&	_light,
							const GBuffer&	_gbuffer,
							const glm::vec3&_view,
							float 			_bias,
							float 			_aperture,
							int 			_nSamples,
							RenderTarget&	_target)
	{
		glUseProgram(program.id);

		glProgramUniform1f(program.id,			biasVar,			_bias);
		glProgramUniform1f(program.id,			apertureVar,		_aperture);
		glProgramUniform1i(program.id,			nSamplesVar,		_nSamples);
		glProgramUniform1i(program.id,			nCascadesVar,		_light.nCascades);
		glProgramUniformMatrix4fv(program.id,	lightViewProjsVar,	_light.nCascades, 	GL_FALSE, &_light.viewprojs[0][0][0]);
		glProgramUniform3f(program.id,			lightDirVar,		_light.direction.x, _light.direction.y, _light.direction.z);
		glProgramUniform3f(program.id,			lightIntensityVar,	_light.intensity.x,	_light.intensity.y,	_light.intensity.z);

		_light.depthTexs.Bind(shadowTexUnit);
		_gbuffer.positionTex.Bind(positionTexUnit);
		_gbuffer.diffuseTex.Bind(diffuseTexUnit);
		_gbuffer.normalTex.Bind(normalTexUnit);
		_target.Draw();

		glf::CheckError("CSMRenderer::Draw");
	}
}

