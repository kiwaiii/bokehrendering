//------------------------------------------------------------------------------
// Depth of Field with Bokeh Rendering
//
// Charles de Rousiers <charles.derousiers@gmail.com>
//------------------------------------------------------------------------------
#include <glf/window.hpp>
#include <glf/scene.hpp>
#include <glf/iomodel.hpp>
#include <glf/buffer.hpp>
#include <glf/pass.hpp>
#include <glf/csm.hpp>
#include <glf/debug.hpp>
#include <glf/sky.hpp>
#include <glf/sh.hpp>
#include <glf/ssao.hpp>
#include <glf/camera.hpp>
#include <glf/wrapper.hpp>
#include <glf/dofprocessor.hpp>
#include <glf/postprocessor.hpp>
#include <glf/utils.hpp>
#include <glf/io/config.hpp>
#include <fstream>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glui/arial12.hpp>
//------------------------------------------------------------------------------
#ifdef WIN32
	#pragma warning( disable : 4996 )
	#pragma warning( disable : 4201 )
#endif
//------------------------------------------------------------------------------
#ifdef WIN32
	#define MAJOR_VERSION	4
	#define MINOR_VERSION	2
#else
	#define MAJOR_VERSION	4
	#define MINOR_VERSION	1	// Create a 4.2 context. Driver bug 285.05.09 on ubuntu 10.11
#endif

//-----------------------------------------------------------------------------
namespace ctx
{
	glf::Camera::Ptr						camera;
	glf::Window 							window(glm::ivec2(1280, 720));
	glui::GlutContext* 						ui;
	bool									drawHelpers = false;
	bool									drawTimings = false;
	bool									drawUI      = false;
}
//-----------------------------------------------------------------------------
namespace
{
	struct SkyParams
	{
		float 								sunTheta;
		float 								sunPhi;
		float 								sunFactor;
		int 								turbidity;
	};

	struct ToneParams
	{
		float 								expToneExposure;
		float								toneExposure;
	};

	struct CSMParams
	{
		int 								nSamples;
		float								bias;
		float								aperture;
		float								blendFactor;
		float								cascadeAlpha;
	};

	struct SSAOParams
	{
		float								beta;
		float								epsilon;
		float								sigma;
		float								kappa;
		float								radius;
		int									nSamples;
		float								sigmaH;
		float								sigmaV;
		int 								nTaps;
	};

	struct DOFParams
	{
		int 								nSamples;
		float								nearStart;
		float								nearEnd;
		float								farStart;
		float								farEnd;
		float								maxCoCRadius;
		float								maxBokehRadius;
		float								lumThreshold;
		float								cocThreshold;
		float								bokehDepthCutoff;
		bool								poissonFiltering;
	};

	struct Application
	{
											Application(int,int);
		glf::ResourceManager				resources;
		glf::SceneManager					scene;

		glf::TimingRenderer					timingRenderer;
		glf::HelperRenderer					helperRenderer;

		glf::GBuffer						gbuffer;
		glf::RenderSurface					renderSurface;
		glf::RenderTarget					renderTarget1;
		glf::RenderTarget					renderTarget2;

		glf::CSMLight						csmLight;
		glf::CSMBuilder						csmBuilder;
		glf::CSMRenderer					csmRenderer;

		glf::CubeMap						cubeMap;
		glf::SkyBuilder						skyBuilder;

		glf::SHLight						shLight;
		glf::SHBuilder						shBuilder;
		glf::SHRenderer						shRenderer;

		glf::SSAOPass						ssaoPass;
		glf::BilateralPass					bilateralPass;

		glf::DOFProcessor					dofProcessor;
		glf::PostProcessor					postProcessor;

		CSMParams 							csmParams;
		SSAOParams 							ssaoParams;
		ToneParams 							toneParams;
		SkyParams							skyParams;
		DOFParams							dofParams;

		bool								updateLighting;
		int									activeBokeh;
		int									activeBuffer;
		int									activeMenu;

		#if ENABLE_BOKEH_STATISTICS
		bool								bokehQuery;
		bool								bokehRecord;
		std::ofstream						bokehFile;
		#endif
	};
	Application*							app;

	const char*								bokehNames[]	= {"Pentagonal","Hexagonal","Circle","Star"};
	struct									bokehType		{ enum Type {BK_PENTAGONAL, BK_HEXAGONAL, BK_CIRCLE,BK_STAR,MAX }; };
	const char*								bufferNames[]	= {"Composition","Position","Normal","Diffuse"};
	struct									bufferType		{ enum Type {GB_COMPOSITION,GB_POSITION,GB_NORMAL,GB_DIFFUSE,MAX }; };
	const char*								menuNames[]		= {"Tone","Sky","CSM","SSAO", "DoF" };
	struct									menuType		{ enum Type {MN_TONE,MN_SKY,MN_CSM,MN_SSAO,MN_DOF,MAX }; };

	Application::Application(int _w, int _h):
	timingRenderer(_w,_h),
	gbuffer(_w,_h),
	renderSurface(_w,_h),
	renderTarget1(_w,_h),
	renderTarget2(_w,_h),
	csmLight(1024,1024,4),
	csmBuilder(),
	csmRenderer(_w,_h),
	cubeMap(),
	skyBuilder(1024),
	shLight(),
	shBuilder(1024),
	shRenderer(_w,_h),
	ssaoPass(_w,_h),
	bilateralPass(_w,_h),
	dofProcessor(_w,_h),
	postProcessor(_w,_h)
	{

		glf::io::ConfigLoader loader;
		glf::io::ConfigNode* root	= loader.Load("config.json");

		glf::io::ConfigNode *dofNode= loader.GetNode(root,"dof");
		dofParams.nSamples 			= loader.GetInt(dofNode,"nSamples",24);
		dofParams.poissonFiltering 	= loader.GetInt(dofNode,"poissonFiltering",false);
		dofParams.nearStart 		= loader.GetFloat(dofNode,"nearStart",0.01f);
		dofParams.nearEnd 			= loader.GetFloat(dofNode,"nearEnd",3.f);
		dofParams.farStart 			= loader.GetFloat(dofNode,"farStart",10.f);
		dofParams.farEnd 			= loader.GetFloat(dofNode,"farEnd",20.f);
		dofParams.maxCoCRadius 		= loader.GetFloat(dofNode,"maxCoCRadius",10.f);
		dofParams.maxBokehRadius 	= loader.GetFloat(dofNode,"maxBokehRadius",15.f);
		dofParams.lumThreshold 		= loader.GetFloat(dofNode,"lumThreshold",5000.f);
		dofParams.cocThreshold 		= loader.GetFloat(dofNode,"cocThreshold",3.5f);
		dofParams.bokehDepthCutoff 	= loader.GetFloat(dofNode,"bokehDepthCutoff",1.f);

		glf::io::ConfigNode *skyNode= loader.GetNode(root,"sky");
		skyParams.turbidity 		= loader.GetInt(skyNode,"turbidity",2);
		skyParams.sunTheta 			= loader.GetFloat(skyNode,"sunTheta",0.63f);
		skyParams.sunPhi 			= loader.GetFloat(skyNode,"sunPhi",5.31f);
		skyParams.sunFactor 		= loader.GetFloat(skyNode,"sunFactor",3.5f);

		glf::io::ConfigNode*csmNode	= loader.GetNode(root,"csm");
		csmParams.nSamples 			= loader.GetInt(csmNode,"nSamples",1);
		csmParams.bias 				= loader.GetFloat(csmNode,"bias",0.0016f);
		csmParams.aperture 			= loader.GetFloat(csmNode,"aperture",0.f);
		csmParams.blendFactor 		= loader.GetFloat(csmNode,"blendFactor",1.f);
		csmParams.cascadeAlpha 		= loader.GetFloat(csmNode,"cascadeAlpha",0.5f);

		glf::io::ConfigNode*ssaoNode= loader.GetNode(root,"ssao");
		ssaoParams.nSamples 		= loader.GetInt(ssaoNode,"nSamples",16);
		ssaoParams.nTaps 			= loader.GetInt(ssaoNode,"nTaps",1);
		ssaoParams.beta 			= loader.GetFloat(ssaoNode,"beta",10e-04f);
		ssaoParams.epsilon 			= loader.GetFloat(ssaoNode,"epsilon",0.0722f);
		ssaoParams.sigma 			= loader.GetFloat(ssaoNode,"sigma",1.f);
		ssaoParams.kappa 			= loader.GetFloat(ssaoNode,"kappa",1.f);
		ssaoParams.radius 			= loader.GetFloat(ssaoNode,"radius",1.f);
		ssaoParams.sigmaH 			= loader.GetFloat(ssaoNode,"sigmaH",1.f);
		ssaoParams.sigmaV 			= loader.GetFloat(ssaoNode,"sigmaV",1.f);

		glf::io::ConfigNode*toneNode= loader.GetNode(root,"tone");
		toneParams.toneExposure 	= loader.GetFloat(toneNode,"expToneExposure",-4.08f);
		toneParams.toneExposure		= pow(10.f,toneParams.expToneExposure);

		glf::io::ConfigNode*dirNode	= loader.GetNode(root,"directory");
		glf::directory::TextureDirectory 	= loader.GetString(dirNode,"textures");
		glf::directory::ShaderDirectory 	= loader.GetString(dirNode,"shaders");
		glf::directory::SceneDirectory 		= loader.GetString(dirNode,"scenes");
		glf::directory::ModelDirectory 		= loader.GetString(dirNode,"models");

		updateLighting				= true;
		activeBokeh					= 1;
		activeBuffer				= 0;
		activeMenu					= 2;
		csmLight.direction			= glm::vec3(0,0,-1);

		#if ENABLE_BOKEH_STATISTICS
		bokehQuery					= false;
		bokehRecord					= false;
		bokehFile.open("BokehPerformances.dat");
		#endif
	}
}
//------------------------------------------------------------------------------
bool resize(int _w, int _h)
{
	return true;
}
//------------------------------------------------------------------------------
bool begin()
{
	assert(glf::CheckGLVersion(MAJOR_VERSION,MINOR_VERSION));

	glClearColor(0.f,0.f,0.f,0.f);
	glClearDepthf(1.0f);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glf::CheckError("begin");

	ctx::camera 			= glf::Camera::Ptr(new glf::HybridCamera());
	glf::manager::timings	= glf::TimingManager::Create();
	glf::manager::helpers	= glf::HelperManager::Create();
	app 					= new Application(ctx::window.Size.x,ctx::window.Size.y);

	glf::io::LoadScene(	"../resources/models/tank/",
						"tank.obj",
						glm::rotate(90.f,1.f,0.f,0.f),
						app->resources,
						app->scene,
						true);

	float farPlane = 2.f * glm::length(app->scene.wBound.pMax - app->scene.wBound.pMin);
	ctx::camera->Perspective(45.f, ctx::window.Size.x, ctx::window.Size.y, 0.1f, farPlane);

	app->renderTarget1.AttachDepthStencil(app->gbuffer.depthTex);
	app->renderTarget2.AttachDepthStencil(app->gbuffer.depthTex);

	glf::manager::helpers->CreateReferential(1.f);

	#if ENABLE_OBJECT_BBOX_HELPERS
	for(unsigned int i=0;i<app->scene.oBounds.size();++i)
	{
		glf::manager::helpers->CreateBound(	app->scene.oBounds[i],
											app->scene.transformations[i]);
	}
	#endif

	glf::CheckError("initScene::end");

	return glf::CheckError("begin");
}
//------------------------------------------------------------------------------
bool end()
{
	return glf::CheckError("end");
}
//------------------------------------------------------------------------------
void gui()
{
	static char labelBuffer[512];
	static glui::Rect none(0,0,200,20);
	static glui::Rect frameRect(0,0,200,10);
	static glui::Rect sliderRect(0, 0, 200, 12);

	ctx::ui->Begin();

		ctx::ui->BeginGroup(glui::Flags::Grow::DOWN_FROM_LEFT);
			ctx::ui->BeginFrame();
			for(int i=0;i<bufferType::MAX;++i)
			{
				bool active = i==app->activeBuffer;
				ctx::ui->CheckButton(none,bufferNames[i],&active);
				app->activeBuffer = active?i:app->activeBuffer;
			}
			ctx::ui->EndFrame();

			ctx::ui->BeginFrame();
			for(int i=0;i<menuType::MAX;++i)
			{
				bool active = i==app->activeMenu;
				ctx::ui->CheckButton(none,menuNames[i],&active);
				app->activeMenu = active?i:app->activeMenu;
			}
			ctx::ui->EndFrame();
			ctx::ui->CheckButton(none,"Helpers",&ctx::drawHelpers);
		ctx::ui->EndGroup();

		bool update = false;
		ctx::ui->BeginGroup(glui::Flags::Grow::DOWN_FROM_RIGHT);
			ctx::ui->BeginFrame();

			if(app->activeMenu == menuType::MN_SKY)
			{
				sprintf(labelBuffer,"Sun (%.2f,%.2f)",app->skyParams.sunTheta,app->skyParams.sunPhi);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,0.f,0.5f*M_PI,&app->skyParams.sunTheta);
				update |= ctx::ui->HorizontalSlider(sliderRect,0.f,2.f*M_PI,&app->skyParams.sunPhi);

				float fturbidity		= float(app->skyParams.turbidity);
				sprintf(labelBuffer,"Turbidity : %d",app->skyParams.turbidity);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,2.f,10.f,&fturbidity);
				app->skyParams.turbidity = int(fturbidity);

				sprintf(labelBuffer,"Factor : %f",app->skyParams.sunFactor);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,100.f,&app->skyParams.sunFactor);

				if(update)
				{
					app->updateLighting = true;
				}
			}

			if(app->activeMenu == menuType::MN_CSM)
			{
				sprintf(labelBuffer,"BlendFactor: %f",app->csmParams.blendFactor);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,0.f,1.f,&app->csmParams.blendFactor);

				sprintf(labelBuffer,"Alpha : %f",app->csmParams.cascadeAlpha);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,1.f,&app->csmParams.cascadeAlpha);

				sprintf(labelBuffer,"Bias: %f",app->csmParams.bias);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,0.f,0.01f,&app->csmParams.bias);

				sprintf(labelBuffer,"Aperture: %f",app->csmParams.aperture);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,0.f,6.f,&app->csmParams.aperture);

				float fnSamples = float(app->csmParams.nSamples);
				sprintf(labelBuffer,"nSamples: %d",app->csmParams.nSamples);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,32.f,&fnSamples);
				app->csmParams.nSamples = int(fnSamples);
			}


			if(app->activeMenu == menuType::MN_SSAO)
			{
				sprintf(labelBuffer,"Beta : %.4f",app->ssaoParams.beta);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,1.f,&app->ssaoParams.beta);

				sprintf(labelBuffer,"Kappa : %.4f",app->ssaoParams.kappa);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,1.f,&app->ssaoParams.kappa);

				sprintf(labelBuffer,"Epsilon : %.4f",app->ssaoParams.epsilon);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,1.f,&app->ssaoParams.epsilon);

				sprintf(labelBuffer,"Sigma : %.4f",app->ssaoParams.sigma);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,1.f,&app->ssaoParams.sigma);

				sprintf(labelBuffer,"Radius : %.4f",app->ssaoParams.radius);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,3.f,&app->ssaoParams.radius);

				float fnSamples = float(app->ssaoParams.nSamples);
				sprintf(labelBuffer,"nSamples : %d",app->ssaoParams.nSamples);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,32.f,&fnSamples);
				app->ssaoParams.nSamples = int(fnSamples);

				sprintf(labelBuffer,"SigmaH : %.4f",app->ssaoParams.sigmaH);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,3.f,&app->ssaoParams.sigmaH);

				sprintf(labelBuffer,"SigmaV : %.4f",app->ssaoParams.sigmaV);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,5.f,&app->ssaoParams.sigmaV);

				float fnTaps = float(app->ssaoParams.nTaps);
				sprintf(labelBuffer,"nTaps : %d",app->ssaoParams.nTaps);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,8.f,&fnTaps);
				app->ssaoParams.nTaps = int(fnTaps);
			}

			if(app->activeMenu == menuType::MN_TONE)
			{
				sprintf(labelBuffer,"Tone Exposure : 10^%.2f",app->toneParams.expToneExposure);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,-6.f,6.f,&app->toneParams.expToneExposure);
				app->toneParams.toneExposure = pow(10.f,app->toneParams.expToneExposure);
			}

			if(app->activeMenu == menuType::MN_DOF)
			{
				sprintf(labelBuffer,"Near Start : %.2f",app->dofParams.nearStart);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.1f,5.f,&app->dofParams.nearStart);

				sprintf(labelBuffer,"Near End : 10^%.2f",app->dofParams.nearEnd);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.1f,5.f,&app->dofParams.nearEnd);

				sprintf(labelBuffer,"Far Start : %.2f",app->dofParams.farStart);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,1.f,100.f,&app->dofParams.farStart);

				sprintf(labelBuffer,"Far End : %.2f",app->dofParams.farEnd);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,1.f,100.f,&app->dofParams.farEnd);

				sprintf(labelBuffer,"Max CoC Radius : %.2f",app->dofParams.maxCoCRadius);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,1.f,30.f,&app->dofParams.maxCoCRadius);

				sprintf(labelBuffer,"Max Bokeh Radius : %.2f",app->dofParams.maxBokehRadius);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,1.f,30.f,&app->dofParams.maxBokehRadius);

				float fnSamples = float(app->dofParams.nSamples);
				sprintf(labelBuffer,"nSamples : %d",app->dofParams.nSamples);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,32.f,&fnSamples);
				app->dofParams.nSamples = int(fnSamples);

				sprintf(labelBuffer,"Lum. Threshold : %.0f",app->dofParams.lumThreshold);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,100.0f,15000.1f,&app->dofParams.lumThreshold);

				sprintf(labelBuffer,"CoC. Threshold : %.2f",app->dofParams.cocThreshold);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,1.0f,30.f,&app->dofParams.cocThreshold);

				sprintf(labelBuffer,"Bokeh depth cutoff : %.2f",app->dofParams.bokehDepthCutoff);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.001f,1.f,&app->dofParams.bokehDepthCutoff);

				ctx::ui->CheckButton(none,"Poisson filtering",&app->dofParams.poissonFiltering);

				// Change bokeh shape
				int previousActiveBokeh = app->activeBokeh;
				for(int i=0;i<bokehType::MAX;++i)
				{
					bool active = i==app->activeBokeh;
					ctx::ui->CheckButton(none,bokehNames[i],&active);
					app->activeBokeh = active?i:app->activeBokeh;
				}
				if(previousActiveBokeh != app->activeBokeh)
				{
					app->dofProcessor.BokehTexture(glf::directory::TextureDirectory + bokehNames[app->activeBokeh] + std::string("Bokeh.png"));
				}

				#if ENABLE_BOKEH_STATISTICS
				if(ctx::ui->Button(none,"Bokeh query"))  app->bokehQuery = true;
				if(ctx::ui->Button(none,"Bokeh record")) app->bokehRecord = true;
				#endif
			}
			ctx::ui->EndFrame();
		ctx::ui->EndGroup();
	ctx::ui->End();

	glf::CheckError("Interface");
}
//------------------------------------------------------------------------------
void display()
{
	glf::manager::timings->StartSection(glf::section::Frame);

	// Optimize far plane
	glm::mat4 projection		= ctx::camera->Projection();
	glm::mat4 view				= ctx::camera->View();
	float nearValue				= ctx::camera->Near();
	glm::vec3 viewPos			= ctx::camera->Eye();

	// Update lighting if needed
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	if(app->updateLighting)
	{
		app->skyBuilder.SetSunFactor(app->skyParams.sunFactor);
		app->skyBuilder.SetPosition(app->skyParams.sunTheta,app->skyParams.sunPhi);
		app->skyBuilder.SetTurbidity(float(app->skyParams.turbidity));
		app->skyBuilder.Update();
		app->shBuilder.Project(app->skyBuilder.skyTexture,app->shLight);
		float sunLuminosity = glm::max(glm::dot(app->skyBuilder.sunIntensity, glm::vec3(0.299f, 0.587f, 0.114f)), 0.0001f);

		glm::vec3 dir;
		dir.x = -sin(app->skyParams.sunTheta)*cos(app->skyParams.sunPhi);
		dir.y = -sin(app->skyParams.sunTheta)*sin(app->skyParams.sunPhi);
		dir.z = -cos(app->skyParams.sunTheta);
		app->csmLight.SetDirection(dir);
		app->csmLight.SetIntensity(glm::vec3(sunLuminosity));

		app->updateLighting = false;
	}

	// Enable writting into the depth buffer
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

	glf::manager::timings->StartSection(glf::section::CsmBuiler);
	app->csmBuilder.Draw(	app->csmLight,
							*ctx::camera,
							app->csmParams.cascadeAlpha,
							app->csmParams.blendFactor,
							app->scene);
	glf::manager::timings->EndSection(glf::section::CsmBuiler);

	// Enable writting into the stencil buffer
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glf::manager::timings->StartSection(glf::section::Gbuffer);
	app->gbuffer.Draw(		projection,
							view,
							app->scene);
	glf::manager::timings->EndSection(glf::section::Gbuffer);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

	// Disable writting into the stencil buffer
	// And activate stencil comparison
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	switch(app->activeBuffer)
	{
		case bufferType::GB_COMPOSITION : 
				glBindFramebuffer(GL_FRAMEBUFFER,app->renderTarget1.framebuffer);
				glClear(GL_COLOR_BUFFER_BIT);

				// Render cube map
				glDisable(GL_STENCIL_TEST);
				glCullFace(GL_FRONT);
				app->cubeMap.Draw(	projection,
									view,
									app->skyBuilder.skyTexture);
				glCullFace(GL_BACK);
				glEnable(GL_STENCIL_TEST);

				// Render sky lighting
				glf::manager::timings->StartSection(glf::section::SkyRender);
				app->shRenderer.Draw(	app->shLight,
										app->gbuffer,
										app->renderTarget1);
				glf::manager::timings->EndSection(glf::section::SkyRender);

				glBindFramebuffer(GL_FRAMEBUFFER,app->renderTarget2.framebuffer);
				glClear(GL_COLOR_BUFFER_BIT);

				// Render ssao render pass
				glf::manager::timings->StartSection(glf::section::SsaoRender);
				app->ssaoPass.Draw(		app->gbuffer,
										view,
										nearValue,
										app->ssaoParams.beta,
										app->ssaoParams.epsilon,
										app->ssaoParams.kappa,
										app->ssaoParams.sigma,
										app->ssaoParams.radius,
										app->ssaoParams.nSamples,
										app->renderTarget2);
				glf::manager::timings->EndSection(glf::section::SsaoRender);
				glBindFramebuffer(GL_FRAMEBUFFER,app->renderTarget1.framebuffer);

				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc( GL_ZERO, GL_SRC_ALPHA); // Do a multiplication between SSAO and sky lighting

				// Render ssao blur pass
				glf::manager::timings->StartSection(glf::section::SsaoBlur);
				app->bilateralPass.Draw(app->renderTarget2.texture,
										app->gbuffer.positionTex,
										view,
										app->ssaoParams.sigmaH,
										app->ssaoParams.sigmaV,
										app->ssaoParams.nTaps,
										app->renderTarget1);
				glf::manager::timings->EndSection(glf::section::SsaoBlur);

				glBlendFunc( GL_ONE, GL_ONE);

				// Render csm/sun light pass
				glf::manager::timings->StartSection(glf::section::CsmRender);
				app->csmRenderer.Draw(	app->csmLight,
										app->gbuffer,
										viewPos,
										app->csmParams.blendFactor,
										app->csmParams.bias,
										app->renderTarget1);
				glf::manager::timings->EndSection(glf::section::CsmRender);

				glBindFramebuffer(GL_FRAMEBUFFER,0);

				glDisable(GL_STENCIL_TEST);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);

				// Render dof processing pass
				glf::manager::timings->StartSection(glf::section::DofProcess);
				app->dofProcessor.Draw(	app->renderTarget1.texture,
										app->gbuffer.positionTex,
										view,
										app->dofParams.nearStart,
										app->dofParams.nearEnd,
										app->dofParams.farStart,
										app->dofParams.farEnd,
										app->dofParams.maxCoCRadius,
										app->dofParams.maxBokehRadius,
										app->dofParams.nSamples,
										app->dofParams.lumThreshold,
										app->dofParams.cocThreshold,
										app->dofParams.bokehDepthCutoff,
										app->dofParams.poissonFiltering,
										app->renderTarget2);
				glf::manager::timings->EndSection(glf::section::DofProcess);
				
				// Record performances
				#if ENABLE_BOKEH_STATISTICS
				if(app->bokehQuery)
				{
					glf::Info("nBokehs : %d",app->dofProcessor.GetDetectedBokehs());
					app->bokehQuery = false;
				}
				if(app->bokehRecord)
				{
					app->bokehFile <<
					app->dofProcessor.GetDetectedBokehs() << " " <<
					glf::manager::timings->GPUTiming(glf::section::DofReset) << " " <<
					glf::manager::timings->GPUTiming(glf::section::DofBlurDepth) << " " <<
					glf::manager::timings->GPUTiming(glf::section::DofDetection) << " " <<
					glf::manager::timings->GPUTiming(glf::section::DofBlur) << " " <<
					glf::manager::timings->GPUTiming(glf::section::DofSynchronization) << " " <<
					glf::manager::timings->GPUTiming(glf::section::DofRendering) << std::endl;
					app->bokehRecord = false;
				}
				#endif

				glBindFramebuffer(GL_FRAMEBUFFER,0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glDisable(GL_STENCIL_TEST);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);

				// Render post processing pass
				glf::manager::timings->StartSection(glf::section::PostProcess);
				app->postProcessor.Draw(app->renderTarget2.texture,
										app->toneParams.toneExposure,
										app->renderTarget1);
				glf::manager::timings->EndSection(glf::section::PostProcess);

				break;
		case bufferType::GB_POSITION :
				glDisable(GL_STENCIL_TEST);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);
				glBindFramebuffer(GL_FRAMEBUFFER,0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				app->renderSurface.Draw(app->gbuffer.positionTex);
				break;
		case bufferType::GB_NORMAL : 
				glDisable(GL_STENCIL_TEST);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				app->renderSurface.Draw(app->gbuffer.normalTex);
				break;
		case bufferType::GB_DIFFUSE : 
				glDisable(GL_STENCIL_TEST);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				app->renderSurface.Draw(app->gbuffer.diffuseTex);
				break;
		default: assert(false);
	}

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if(ctx::drawHelpers)
		app->helperRenderer.Draw(projection,view,glf::manager::helpers->helpers);
	if(ctx::drawUI) 
		gui();
	if(ctx::drawTimings) 
		app->timingRenderer.Draw(*glf::manager::timings);
	glDisable(GL_BLEND);

	glf::CheckError("display");
	glf::SwapBuffers();

	glf::manager::timings->EndSection(glf::section::Frame);
}
//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	glf::Info("Start");
	if(glf::Run(argc, 
				argv,
				glm::ivec2(ctx::window.Size.x,ctx::window.Size.y), 
				MAJOR_VERSION, 
				MINOR_VERSION))
				return 0;
	return 1;
}
