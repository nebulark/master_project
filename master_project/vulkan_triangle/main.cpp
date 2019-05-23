// raytrace_software.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include "common/Deleters.hpp"
#include "common/VulkanDebug.hpp"


vk::UniqueInstance create_vulkan_instance(gsl::span<const char*> enabledLayers, gsl::span<const char*> enabledExtensions)
{
	vk::ApplicationInfo application_info("Hello Triangle", 1, "My Engine", 1, VK_API_VERSION_1_1);


	vk::InstanceCreateInfo instanceCreateInfo({}, &application_info,
		static_cast<uint32_t>(std::size(enabledLayers)), std::data(enabledLayers),
		static_cast<uint32_t>(std::size(enabledExtensions)), std::data(enabledExtensions));


	vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfo);

		
	
	return instance;
}

std::vector<const char*> GetSdlExtensions(SDL_Window* window)
{
	unsigned int sdl_extension_count = -1;
	{
		const bool success = SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, nullptr);
	}

	std::vector<const char*> enabledExtensions;
	enabledExtensions.resize(sdl_extension_count);
	{
		const bool success = SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, enabledExtensions.data());
		assert(success);
	}

	return enabledExtensions;
}

// Instance must live longer than the returned object!!!
vk::UniqueSurfaceKHR CreateSurface(vk::Instance& instance, SDL_Window* window)
{

	VkSurfaceKHR vksurface = nullptr;
	const bool success = SDL_Vulkan_CreateSurface(window, instance, &vksurface);
	assert(success);

	return vk::UniqueSurfaceKHR(
		vk::SurfaceKHR(vksurface),
		vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderStatic>(instance)
	);
}

bool SupportsValidationLayers(gsl::span<const char*> layerNames)
{
	const std::vector<vk::LayerProperties> availableLayerProperties = vk::enumerateInstanceLayerProperties();
	return std::all_of(layerNames.cbegin(), layerNames.cend(), [&availableLayerProperties](const char* layerName)
	{
		return std::any_of(availableLayerProperties.cbegin(), availableLayerProperties.cend(), [layerName](const vk::LayerProperties& availableLayerProperty)
		{
			return std::strcmp(availableLayerProperty.layerName, layerName) == 0;
		});
	});

}

VkBool32 DebugUtilsCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	vk::DebugUtilsMessageSeverityFlagBitsEXT severity = static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity);
	vk::DebugUtilsMessageTypeFlagsEXT types(messageTypes);

	std::cerr << vk::to_string(severity) << ' ' << vk::to_string(types) << ' ' << pCallbackData->pMessage << '\n';
	
	constexpr VkBool32 shouldAbortFunctionCall = VK_TRUE;
	return shouldAbortFunctionCall;
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
			SDL_WINDOW_VULKAN                 // flags - see below
			)
		};


		const char* enabledValidationLayers[] =
		{
			// Enable standard validation layer to find as much errors as possible!
			"VK_LAYER_KHRONOS_validation"
		};

		std::vector<const char*> enabledExtensions = GetSdlExtensions(window.get());
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);


		assert(SupportsValidationLayers(enabledValidationLayers));
		vk::UniqueInstance instance = create_vulkan_instance(enabledValidationLayers, enabledExtensions);


		VulkanDebugExtension::LoadDebugUtilsMessengerExtension(instance.get());

		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags = vk::DebugUtilsMessageSeverityFlagsEXT()
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
			;

		vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags = vk::DebugUtilsMessageTypeFlagsEXT()
			| vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
			| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
			| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
			;

		vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info(vk::DebugUtilsMessengerCreateFlagsEXT(), severityFlags, messageTypeFlags, &DebugUtilsCallback);
		vk::UniqueDebugUtilsMessengerEXT messenger = instance->createDebugUtilsMessengerEXTUnique(debug_utils_create_info);

		std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();
		assert(!physicalDevices.empty());

		vk::UniqueSurfaceKHR surface = CreateSurface(instance.get(), window.get());	
	}
	SDL_Quit();
		

	return 0;
	
}


