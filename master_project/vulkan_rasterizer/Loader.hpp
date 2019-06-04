#pragma once
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include <vulkan/vulkan.hpp>

namespace Loader
{


	void LoadImageFromFilesToGpu(vk::Device device, VmaAllocator allocator, vk::Queue transferQueue);

	struct ImageGpuLoaderResult
	{
		vk::Buffer StagingBuffer;
		vk::UniqueImage ImageResult;
		vk::Fence OperationFinished;
	};

}
