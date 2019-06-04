#pragma once
#include <vulkan/vulkan.hpp>
namespace DebugUtils
{
	vk::UniqueDebugUtilsMessengerEXT CreateDebugUtilsMessenger(vk::Instance instance);
}
