#include "pch.h"

#include "VulkanDebug.hpp"


namespace
{
	// These will be loaded Dynamically

	// old interface
	PFN_vkCreateDebugReportCallbackEXT  pfnVkCreateDebugReportCallbackEXT = nullptr;
	PFN_vkDestroyDebugReportCallbackEXT pfnVkDestroyDebugReportCallbackEXT = nullptr;

	// new interface
	PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT = nullptr;
	PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT = nullptr;
}

void VulkanDebugExtension::LoadDebugUtilsMessengerExtension(const vk::Instance& instance)
{
	assert(pfnVkCreateDebugUtilsMessengerEXT == nullptr);
	pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));

	assert(pfnVkDestroyDebugUtilsMessengerEXT == nullptr);
	pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

	assert(pfnVkCreateDebugUtilsMessengerEXT && "Required " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " extension");
}

void VulkanDebugExtension::LoadDebugReportCallbackExtension(const vk::Instance& instance)
{
	assert(pfnVkCreateDebugReportCallbackEXT == nullptr);
	pfnVkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(instance.getProcAddr("vkCreateDebugReportCallbackEXT"));

	assert(pfnVkDestroyDebugReportCallbackEXT == nullptr);
	pfnVkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(instance.getProcAddr("vkDestroyDebugReportCallbackEXT"));

	assert(pfnVkCreateDebugReportCallbackEXT && "Requires " VK_EXT_DEBUG_REPORT_EXTENSION_NAME " extension");
}




// Define the Debug Report functions, which are delcared in the vulkan header and are used by the DispatchLoaderStatic
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
		assert(pfnVkCreateDebugUtilsMessengerEXT && "Need to call VulkanDebugReportExtension::LoadDebugUtilsMessengerExtension before this can be used");
		return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
	}

	VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
		VkInstance                                  instance,
		VkDebugUtilsMessengerEXT                    messenger,
		const VkAllocationCallbacks* pAllocator)
	{
		assert(pfnVkDestroyDebugUtilsMessengerEXT && "Need to call VulkanDebugReportExtension::LoadDebugUtilsMessengerExtension before this can be used");
		pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
	}


	VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
	{
		assert(pfnVkCreateDebugReportCallbackEXT && "Need to call VulkanDebugReportExtension::LoadDebugReportCallbackExtension before this can be used");
		return ::pfnVkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
	}

	VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
	{
		assert(pfnVkDestroyDebugReportCallbackEXT && "Need to call VulkanDebugReportExtension::LoadDebugReportCallbackExtension before this can be used");
		::pfnVkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
	}	
}


