#pragma once
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include <vulkan/vulkan.hpp>

struct VmaBuffer
{
	vk::Buffer buffer;
	VmaAllocation allocation;
};

namespace VmaUtils
{
	


}
