// raytrace_software.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"



#include "common/Deleters.hpp"

vk::UniqueInstance create_vulkan_instance()
{
	vk::ApplicationInfo application_info("Hello Triangle", 1, "My Engine", 1, VK_API_VERSION_1_1);


	const char* enabledLayers[] = 
	{
		// Enable standard validation layer to find as much errors as possible!
		"VK_LAYER_KHRONOS_validation" 
	};

	
	const char* enabledExtensions[] =
	{
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	};

	vk::InstanceCreateInfo instanceCreateInfo({}, &application_info,
		std::size(enabledLayers), enabledLayers,
		std::size(enabledExtensions), enabledExtensions);


	vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfo);

		
	
	return instance;
}

struct ReportCallbacks
{
	PFN_vkCreateDebugReportCallbackEXT  pfnVkCreateDebugReportCallbackEXT;
	PFN_vkDestroyDebugReportCallbackEXT pfnVkDestroyDebugReportCallbackEXT;
};

ReportCallbacks LoadReportCallbacks(vk::Instance* instance)
{
	ReportCallbacks result;
	result.pfnVkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(instance->getProcAddr("vkCreateDebugReportCallbackEXT"));
	result.pfnVkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(instance->getProcAddr("vkDestroyDebugReportCallbackEXT"));
}




int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	{
		WindowPtr window{
				SDL_CreateWindow(
				"An SDL2 window",                  // window title
				SDL_WINDOWPOS_UNDEFINED,           // initial x position
				SDL_WINDOWPOS_UNDEFINED,           // initial y position
				1920,                               // width, in pixels
				1080,                               // height, in pixels
				SDL_WINDOW_OPENGL                 // flags - see below
			)
		};

		

	
	}
	SDL_Quit();
		

	return 0;
	
}


