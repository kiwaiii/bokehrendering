#ifndef GLF_DEBUG_HPP
#define GLF_DEBUG_HPP

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <glf/helper.hpp>
#include <glf/timing.hpp>

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define ENABLE_CSM_HELPERS				1
#define ENABLE_OBJECT_BBOX_HELPERS		1
#define ENABLE_SCENE_BBOX_HELPERS		0
#define ENABLE_OBJECT_TBN_HELPERS		0
//------------------------------------------------------------------------------
#define ENABLE_LOAD_NORMAL_MAP			0
#define ENABLE_ANISOSTROPIC_FILTERING	1
//------------------------------------------------------------------------------
#define ENABLE_DOF_PASS_TIMING			1
#define ENABLE_GPU_PASSES_TIMING		1
#define ENABLE_GPU_FRAME_TIMING			0
//------------------------------------------------------------------------------
#define ENABLE_BOKEH_STATISTICS			1
//------------------------------------------------------------------------------
#define ENABLE_CHECK_ERROR				1
#define ENABLE_VERBOSE_PROGRAM 			0

namespace glf
{
	namespace manager
	{
		// Global managers 
		// Have to be allocated at application's initialization
		extern HelperManager::Ptr helpers;
		extern TimingManager::Ptr timings;
	}
}

#endif
