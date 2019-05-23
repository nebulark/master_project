#include "pch.h"

#include "VulkanDebugReport.hpp"


namespace
{
	// These will be loaded Dynamically
	PFN_vkCreateDebugReportCallbackEXT  pfnVkCreateDebugReportCallbackEXT = nullptr;
	PFN_vkDestroyDebugReportCallbackEXT pfnVkDestroyDebugReportCallbackEXT = nullptr;
}

void VulkanDebugReportExtension::Load(const vk::Instance& instance)
{
	assert(pfnVkCreateDebugReportCallbackEXT == nullptr);
	pfnVkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(instance.getProcAddr("vkCreateDebugReportCallbackEXT"));

	assert(pfnVkDestroyDebugReportCallbackEXT == nullptr);
	pfnVkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(instance.getProcAddr("vkDestroyDebugReportCallbackEXT"));
}

// Define the Debug Report functions, which are delcared in the vulkan header and are used by the DispatchLoaderStatic
// They are defined in vulkan_core.h, but have not declaration
// They need to be in global namespace and match the definition in vulkan_core.h (thats why it is in extern "C")
extern "C"
{
	VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
	{
		assert(pfnVkCreateDebugReportCallbackEXT && "Need to call VulkanDebugReportExtension::Load before this can be used");
		return ::pfnVkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
	}

	VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
	{
		assert(pfnVkDestroyDebugReportCallbackEXT && "Need to call VulkanDebugReportExtension::Load before this can be used");
		::pfnVkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
	}
}


