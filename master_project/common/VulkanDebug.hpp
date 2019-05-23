#pragma once

namespace vk
{
	class Instance;
}

namespace VulkanDebugExtension
{
	// new Extension
	void LoadDebugUtilsMessengerExtension(const vk::Instance& instance);

	// old Extension
	void LoadDebugReportCallbackExtension(const vk::Instance& instance);	
}