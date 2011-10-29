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
#define ENABLE_DOF_PASS_TIMING			1
#define ENABLE_GPU_PASSES_TIMING		0
#define ENABLE_GPU_FRAME_TIMING			0

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
