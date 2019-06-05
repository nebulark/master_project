#include "pch.h"

#include "VulkanDebug.hpp"


namespace
{
	// These will be loaded Dynamically

	PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT = nullptr;
	PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT = nullptr;

	PFN_vkSetDebugUtilsObjectNameEXT pfnVkSetDebugUtilsObjectNameEXT = nullptr;
}

void VulkanDebug::LoadDebugUtilsExtension(const vk::Instance& instance)
{
	assert(pfnVkCreateDebugUtilsMessengerEXT == nullptr);
	pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));

	assert(pfnVkDestroyDebugUtilsMessengerEXT == nullptr);
	pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

	assert(pfnVkSetDebugUtilsObjectNameEXT == nullptr);
	pfnVkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(instance.getProcAddr("vkSetDebugUtilsObjectNameEXT"));

	assert(pfnVkCreateDebugUtilsMessengerEXT && "Required " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " extension");
}



// Define the Debug  functions, which are delcared in the vulkan header and are used by the DispatchLoaderStatic
// They are defined in vulkan_core.h, but have not declaration
// They need to be in global namespace and match the definition in vulkan_core.h (thats why it is in extern "C") to successfully link
extern "C"
{
	VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
		VkInstance                                  instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pMessenger)
	{
		assert(pfnVkCreateDebugUtilsMessengerEXT && "Need to call VulkanDebug::LoadDebugUtilsExtension before this can be used");
		return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
	}

	VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
		VkInstance                                  instance,
		VkDebugUtilsMessengerEXT                    messenger,
		const VkAllocationCallbacks* pAllocator)
	{
		assert(pfnVkDestroyDebugUtilsMessengerEXT && "Need to call VulkanDebug::LoadDebugUtilsExtension before this can be used");
		pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
	}

	VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
	{
		assert(pfnVkSetDebugUtilsObjectNameEXT && "Need to call VulkanDebug::LoadDebugUtilsExtension before this can be used");
		return ::pfnVkSetDebugUtilsObjectNameEXT(device, pNameInfo);
	}
}


