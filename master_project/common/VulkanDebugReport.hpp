#pragma once

namespace vk
{
	class Instance;
}

namespace VulkanDebugReportExtension
{
	void Load(const vk::Instance& instance);
}