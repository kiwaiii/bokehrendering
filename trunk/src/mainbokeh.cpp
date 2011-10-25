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
#include <glf/sky.hpp>
#include <glf/sh.hpp>
#include <glf/ssao.hpp>
#include <glf/camera.hpp>
#include <glf/wrapper.hpp>
#include <glf/helper.hpp>
#include <glf/dof2.hpp>
#include <glf/postprocessor.hpp>
#include <glf/utils.hpp>
#include <fstream>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
//------------------------------------------------------------------------------
#ifdef WIN32
	#pragma warning( disable : 4996 )
#endif
//------------------------------------------------------------------------------
#define MAJOR_VERSION	4
#define MINOR_VERSION	1
#define BBOX_SCENE		1


//-----------------------------------------------------------------------------
namespace ctx
{
	glf::Camera::Ptr						camera;
	glf::Window 							window(glm::ivec2(1280, 720));
	glui::GlutContext* 						ui;
	bool									drawHelpers = false;
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
		float 								expBloomExposure;
		float								toneExposure;
		float								bloomExposure;
		float								bloomMagnitude;
		float								bloomSigma;
		int									bloomTaps;
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
		float								attenuation;
		float								areaFactor;
	};

	struct Application
	{
											Application(int,int);
		glf::ResourceManager				resources;
		glf::SceneManager					scene;

		glf::HelperManager					helpers;
		glf::HelperRenderer					helperRenderer;

		glf::GBuffer						gbuffer;
		glf::Surface						surface;
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

		int									activeBuffer;
		int									activeMenu;
	};
	Application*							app;

	const char*								bufferNames[]	= {"Composition","Position","Normal","Diffuse","Specular" };
	struct									bufferType		{ enum Type {GB_COMPOSITION,GB_POSITION,GB_NORMAL,GB_DIFFUSE,GB_SPECULAR,MAX }; };
	const char*								menuNames[]		= {"Tone","Sky","CSM","SSAO", "DoF" };
	struct									menuType		{ enum Type {MN_TONE,MN_SKY,MN_CSM,MN_SSAO,MN_DOF,MAX }; };

	Application::Application(int _w, int _h):
	gbuffer(_w,_h),
	surface(_w,_h),
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
		ssaoParams.beta				= 10e-04;
		ssaoParams.epsilon			= 0.0722;
		ssaoParams.sigma			= 1.f;
		ssaoParams.kappa			= 1.f;
		ssaoParams.radius			= 1.0f;
		ssaoParams.nSamples			= 24;
		ssaoParams.sigmaH			= 1.f;
		ssaoParams.sigmaV			= 1.f;
		ssaoParams.nTaps			= 4;

		toneParams.expToneExposure 	=-4.08f;
		toneParams.expBloomExposure	=-4.33f;
		toneParams.toneExposure		= pow(10.f,toneParams.expToneExposure);
		toneParams.bloomExposure	= pow(10.f,toneParams.expBloomExposure);
		toneParams.bloomMagnitude	= 0.f;
		toneParams.bloomSigma		= 3.f;
		toneParams.bloomTaps		= 4;

		csmParams.nSamples			= 1;
		csmParams.bias				= 0.0016f;
		csmParams.aperture			= 0.0f;
		csmParams.blendFactor		= 1.f;
		csmParams.cascadeAlpha		= 0.5f;

		skyParams.sunTheta			= 0.63;
		skyParams.sunPhi			= 5.31;
		skyParams.turbidity			= 2;
		skyParams.sunFactor			= 3.5f;

		dofParams.nearStart			= 0.01;
		dofParams.nearEnd			= 3.00;
		dofParams.farStart			= 10.f;
		dofParams.farEnd			= 20.f;
		dofParams.maxCoCRadius		= 10.f;
		dofParams.maxBokehRadius	= 10.f;
		dofParams.nSamples			= 24;
		dofParams.lumThreshold		= 5000.f;
		dofParams.cocThreshold		= 3.5f;
		dofParams.attenuation		= 5.f;
		dofParams.areaFactor		= 1.f;

		activeBuffer				= 0;
		activeMenu					= 2;

		csmLight.direction			= glm::vec3(0,0,-1);
	}
}

//------------------------------------------------------------------------------
void UpdateLight()
{
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);

	app->skyBuilder.SetSunFactor(app->skyParams.sunFactor);
	app->skyBuilder.SetPosition(app->skyParams.sunTheta,app->skyParams.sunPhi);
	app->skyBuilder.SetTurbidity(app->skyParams.turbidity);
	app->skyBuilder.Update();
	app->shBuilder.Project(app->skyBuilder.skyTexture,app->shLight);
	float sunLuminosity = glm::max(glm::dot(app->skyBuilder.sunIntensity, glm::vec3(0.299f, 0.587f, 0.114f)), 0.0001f);

	glm::vec3 dir;
	dir.x = -sin(app->skyParams.sunTheta)*cos(app->skyParams.sunPhi);
	dir.y = -sin(app->skyParams.sunTheta)*sin(app->skyParams.sunPhi);
	dir.z = -cos(app->skyParams.sunTheta);
	app->csmLight.SetDirection(dir);
	app->csmLight.SetIntensity(glm::vec3(sunLuminosity));
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

	app = new Application(ctx::window.Size.x,ctx::window.Size.y);

	glf::io::LoadScene(	"../resources/models/tank/",
						"tank.obj",
						glm::rotate(90.f,1.f,0.f,0.f),
						app->resources,
						app->scene,
						app->helpers,
						true);

	float farPlane = 2.f * glm::length(app->scene.wBound.pMax - app->scene.wBound.pMin);

	ctx::camera = glf::Camera::Ptr(new glf::HybridCamera());
	ctx::camera->Perspective(45.f, ctx::window.Size.x, ctx::window.Size.y, 0.1f, farPlane);

	app->renderTarget1.AttachDepthStencil(app->gbuffer.depthTex);
	app->renderTarget2.AttachDepthStencil(app->gbuffer.depthTex);

	app->helpers.CreateReferential(1.f);
	#if BBOX_SCENE
	for(unsigned int i=0;i<app->scene.oBounds.size();++i)
	{
		app->helpers.CreateBound(	app->scene.oBounds[i],
									app->scene.transformations[i]);
	}
	#endif

	UpdateLight();

	glf::CheckError("initScene::end");

	return glf::CheckError("begin");
}
//------------------------------------------------------------------------------
bool end()
{
	return glf::CheckError("end");
}
//------------------------------------------------------------------------------
void interface()
{
	static char labelBuffer[512];
	static glui::Rect none(0,0,200,20);
	static glui::Rect frameRect(0,0,200,10);
	static int frameLayout = glui::Flags::Layout::DEFAULT;
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
				app->skyParams.turbidity = fturbidity;

				sprintf(labelBuffer,"Factor : %f",app->skyParams.sunFactor);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,100.f,&app->skyParams.sunFactor);

				if(update)
				{
					UpdateLight();
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

				float fnSamples = app->csmParams.nSamples;
				sprintf(labelBuffer,"nSamples: %d",app->csmParams.nSamples);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,32.f,&fnSamples);
				app->csmParams.nSamples = fnSamples;
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

				float fnSamples = app->ssaoParams.nSamples;
				sprintf(labelBuffer,"nSamples : %d",app->ssaoParams.nSamples);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,32.f,&fnSamples);
				app->ssaoParams.nSamples = fnSamples;

				sprintf(labelBuffer,"SigmaH : %.4f",app->ssaoParams.sigmaH);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,3.f,&app->ssaoParams.sigmaH);

				sprintf(labelBuffer,"SigmaV : %.4f",app->ssaoParams.sigmaV);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,5.f,&app->ssaoParams.sigmaV);

				float fnTaps = app->ssaoParams.nTaps;
				sprintf(labelBuffer,"nTaps : %d",app->ssaoParams.nTaps);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,8.f,&fnTaps);
				app->ssaoParams.nTaps = fnTaps;
			}

			if(app->activeMenu == menuType::MN_TONE)
			{
				static float expToneExposure = -4;
				sprintf(labelBuffer,"Tone Exposure : 10^%.2f",app->toneParams.expToneExposure);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,-6.f,6.f,&app->toneParams.expToneExposure);
				app->toneParams.toneExposure = pow(10.f,app->toneParams.expToneExposure);

				static float expBloomExposure = -4;
				sprintf(labelBuffer,"Bloom Exposure : 10^%.2f",app->toneParams.expBloomExposure);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,-6.f,6.f,&app->toneParams.expBloomExposure);
				app->toneParams.bloomExposure = pow(10.f,app->toneParams.expBloomExposure);

				sprintf(labelBuffer,"Bloom Magnitude : %.2f",app->toneParams.bloomMagnitude);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,1.f,&app->toneParams.bloomMagnitude);

				sprintf(labelBuffer,"Bloom Sigma : %.2f",app->toneParams.bloomSigma);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.f,3.f,&app->toneParams.bloomSigma);

				float fbloomTaps = app->toneParams.bloomTaps;
				sprintf(labelBuffer,"Bloom Taps : %d",app->toneParams.bloomTaps);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,1.f,6.f,&fbloomTaps);
				app->toneParams.bloomTaps = fbloomTaps;
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

				float fnSamples = app->dofParams.nSamples;
				sprintf(labelBuffer,"nSamples : %d",app->dofParams.nSamples);
				ctx::ui->Label(none,labelBuffer);
				update |= ctx::ui->HorizontalSlider(sliderRect,1.f,32.f,&fnSamples);
				app->dofParams.nSamples = fnSamples;

				sprintf(labelBuffer,"Lum. Threshold : %.0f",app->dofParams.lumThreshold);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,100.0f,15000.1f,&app->dofParams.lumThreshold);

				sprintf(labelBuffer,"CoC. Threshold : %.2f",app->dofParams.cocThreshold);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,1.0f,30.f,&app->dofParams.cocThreshold);

				sprintf(labelBuffer,"Attenuation : %.2f",app->dofParams.attenuation);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,1.0f,10.f,&app->dofParams.attenuation);

				sprintf(labelBuffer,"Area Factor : %.2f",app->dofParams.areaFactor);
				ctx::ui->Label(none,labelBuffer);
				ctx::ui->HorizontalSlider(sliderRect,0.001f,1.f,&app->dofParams.areaFactor);
			}
			ctx::ui->EndFrame();
		ctx::ui->EndGroup();

	ctx::ui->End();

	glf::CheckError("Interface");
}
//------------------------------------------------------------------------------
void display()
{
	// Optimize far plane
	glm::mat4 projection		= ctx::camera->Projection();
	glm::mat4 view				= ctx::camera->View();
	float near					= ctx::camera->Near();
	glm::vec3 viewPos			= ctx::camera->Eye();


	// Enable writting into the depth buffer
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

	app->csmBuilder.Draw(	app->csmLight,
							*ctx::camera,
							app->csmParams.cascadeAlpha,
							app->csmParams.blendFactor,
							app->scene,
							app->helpers);

	// Enable writting into the stencil buffer
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	app->gbuffer.Draw(		projection,
							view,
							app->scene);

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

				glDisable(GL_STENCIL_TEST);
				glCullFace(GL_FRONT);
				app->cubeMap.Draw(	projection,
									view,
									app->skyBuilder.skyTexture);
				glCullFace(GL_BACK);
				glEnable(GL_STENCIL_TEST);

				app->shRenderer.Draw(	app->shLight,
										app->gbuffer,
										app->renderTarget1);


				glBindFramebuffer(GL_FRAMEBUFFER,app->renderTarget2.framebuffer);
				glClear(GL_COLOR_BUFFER_BIT);

				app->ssaoPass.Draw(		app->gbuffer,
										view,
										near,
										app->ssaoParams.beta,
										app->ssaoParams.epsilon,
										app->ssaoParams.kappa,
										app->ssaoParams.sigma,
										app->ssaoParams.radius,
										app->ssaoParams.nSamples,
										app->renderTarget2);

				glBindFramebuffer(GL_FRAMEBUFFER,app->renderTarget1.framebuffer);

				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc( GL_ZERO, GL_SRC_ALPHA); // Do a multiplication between SSAO and sky lighting

				app->bilateralPass.Draw(app->renderTarget2.texture,
										app->gbuffer.positionTex,
										view,
										app->ssaoParams.sigmaH,
										app->ssaoParams.sigmaV,
										app->ssaoParams.nTaps,
										app->renderTarget1);

				glBlendFunc( GL_ONE, GL_ONE);

				app->csmRenderer.Draw(	app->csmLight,
										app->gbuffer,
										viewPos,
										app->csmParams.blendFactor,
										app->csmParams.bias,
										app->renderTarget1);

				#if 0
				glBindFramebuffer(GL_FRAMEBUFFER,app->renderTarget2.framebuffer);
				glClear(GL_COLOR_BUFFER_BIT);

				glDisable(GL_STENCIL_TEST);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//				glDisable(GL_BLEND);

				app->dofProcessor.Draw(	app->renderTarget1.texture,
										app->gbuffer.positionTex,
										view,
										app->dofParams.nearStart,
										app->dofParams.nearEnd,
										app->dofParams.farStart,
										app->dofParams.farEnd,
										app->dofParams.maxCoCRadius,
										app->dofParams.nSamples,
										app->dofParams.lumThreshold,
										app->dofParams.cocThreshold,
										app->dofParams.attenuation,
										app->dofParams.areaFactor,
										app->renderTarget2);

				glBindFramebuffer(GL_FRAMEBUFFER,0);

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				app->postProcessor.Apply(app->renderTarget2.texture,
//				app->postProcessor.Apply(app->dofProcessor.hblurPass.blurTex,
//				app->postProcessor.Apply(app->dofProcessor.cocPass.cocDepthTex,
										 app->toneParams.toneExposure);
				#endif

				#if 1
				glBindFramebuffer(GL_FRAMEBUFFER,0);
				glDisable(GL_STENCIL_TEST);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
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
										app->dofParams.attenuation,
										app->dofParams.areaFactor,
										app->renderTarget2);

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//				app->postProcessor.Apply(app->dofProcessor.detectionTex,
				app->postProcessor.Apply(app->dofProcessor.renderingTex,
//				app->postProcessor.Apply(app->dofProcessor.cocPass.cocDepthTex,
										 app->toneParams.toneExposure);
				#endif

				break;
		case bufferType::GB_POSITION : 
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				app->surface.Draw(app->gbuffer.positionTex);
				break;
		case bufferType::GB_NORMAL : 
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				app->surface.Draw(app->gbuffer.normalTex);
				break;
		case bufferType::GB_DIFFUSE : 
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				app->surface.Draw(app->gbuffer.diffuseTex);
				break;
		case bufferType::GB_SPECULAR : 
				assert(false);
				break;
		default: assert(false);
	}

	if(ctx::drawHelpers)
		app->helperRenderer.Draw(projection,view,app->helpers.helpers);

	if(ctx::drawUI)
		interface();

	glf::CheckError("display");
	glf::SwapBuffers();
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


/*
static bool isinit = false;
static glm::vec4* data= NULL;
if(!isinit)
{
	data = new glm::vec4[ctx::window.Size.x*ctx::window.Size.y];
	isinit = true;
}
glBindTexture(app->renderTarget2.texture.target,app->renderTarget2.texture.id);
glGetTexImage(app->renderTarget2.texture.target,0,GL_RGBA,GL_FLOAT,data);
glBindTexture(app->renderTarget2.texture.target,0);

glf::Info("(0,0) = (%f,%f)",data[0].x,data[0].y);
glf::Info("(1,0) = (%f,%f)",data[1].x,data[1].y);
glf::Info("(2,0) = (%f,%f)",data[2].x,data[2].y);
glf::Info("(3,0) = (%f,%f)",data[3].x,data[3].y);

glf::Info("(0,1) = (%f,%f)",data[0+ctx::window.Size.x].x,data[0+ctx::window.Size.x].y);
glf::Info("(1,1) = (%f,%f)",data[1+ctx::window.Size.x].x,data[1+ctx::window.Size.x].y);
glf::Info("(2,1) = (%f,%f)",data[2+ctx::window.Size.x].x,data[2+ctx::window.Size.x].y);
glf::Info("(3,1) = (%f,%f)",data[3+ctx::window.Size.x].x,data[3+ctx::window.Size.x].y);

glf::Info("(0,2) = (%f,%f)",data[0+2*ctx::window.Size.x].x,data[0+2*ctx::window.Size.x].y);
glf::Info("(1,2) = (%f,%f)",data[1+2*ctx::window.Size.x].x,data[1+2*ctx::window.Size.x].y);
glf::Info("(2,2) = (%f,%f)",data[2+2*ctx::window.Size.x].x,data[2+2*ctx::window.Size.x].y);
glf::Info("(3,2) = (%f,%f)",data[3+2*ctx::window.Size.x].x,data[3+2*ctx::window.Size.x].y);
*/

