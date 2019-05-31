#pragma once
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include <memory>
#include <vulkan/vulkan.hpp>

namespace VmaRAII
{
	struct VmaAllocatorDeleter
	{
		void operator()(VmaAllocator allocator)
		{
			vmaDestroyAllocator(allocator);
		}
	};

	std::unique_ptr<VmaAllocator_T, VmaAllocatorDeleter> VmaAllocatorUnique(const VmaAllocatorCreateInfo& createInfo)
	{
		VmaAllocator alloc;
		VkResult vkResult = vmaCreateAllocator(&createInfo, &alloc);
		if (vkResult != VkResult::VK_SUCCESS)
		{
			vk::Result result = reinterpret_cast<vk::Result&>(vkResult);
			throw std::runtime_error(vk::to_string(result));
		}
		return std::unique_ptr<VmaAllocator_T, VmaAllocatorDeleter>(alloc);
	}

	using UniqueVmaAllocator = 	std::unique_ptr<VmaAllocator_T, VmaAllocatorDeleter>;

}
