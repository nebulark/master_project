// raytrace_software.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"



#include "common/Deleters.hpp"
#include "common/VulkanDebugReport.hpp"

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
		static_cast<uint32_t>(std::size(enabledLayers)), enabledLayers,
		static_cast<uint32_t>(std::size(enabledExtensions)), enabledExtensions);


	vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfo);

		
	
	return instance;
}

VkBool32 debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*objectType*/, uint64_t /*object*/, size_t /*location*/, int32_t /*messageCode*/, const char* /*pLayerPrefix*/, const char* pMessage, void* /*pUserData*/)
{
	switch (flags)
	{
	case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
		std::cerr << "INFORMATION: ";
		break;
	case VK_DEBUG_REPORT_WARNING_BIT_EXT:
		std::cerr << "WARNING: ";
		break;
	case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
		std::cerr << "PERFORMANCE WARNING: ";
		break;
	case VK_DEBUG_REPORT_ERROR_BIT_EXT:
		std::cerr << "ERROR: ";
		break;
	case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
		std::cerr << "DEBUG: ";
		break;
	default:
		std::cerr << "unknown flag (" << flags << "): ";
		break;
	}
	std::cerr << pMessage << std::endl;
	return VK_TRUE;
}


int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	{

		vk::UniqueInstance instance = create_vulkan_instance();
		VulkanDebugReportExtension::Load(instance.get());
		vk::DebugReportFlagsEXT flags(vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eError);
		vk::DebugReportCallbackCreateInfoEXT report_callback_create_info(flags, &debugReportCallback);
		vk::UniqueDebugReportCallbackEXT debugReportCallback = instance->createDebugReportCallbackEXTUnique(report_callback_create_info);







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


