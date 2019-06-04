#include "pch.hpp"
#include "Loader.hpp"




void Loader::LoadImageFromFilesToGpu(vk::Device device, VmaAllocator allocator, vk::Queue transferQueue)
{
#if 0
	vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo{}
			.setCommandBufferCount(1)
			.setCommandPool(m_transferQueuePool.get())
			.setLevel(vk::CommandBufferLevel::ePrimary)
			;

		vk::UniqueCommandBuffer loadBuffer = VulkanUtils::allocSingleCommandBuffer(m_device.get(), m_transferQueuePool.get());
		loadBuffer->begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		{

		}

		loadBuffer->end();

		// Todo keep buffer around and wait only when required instead of immediately
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&(loadBuffer.get()))
			;
			m_transferQueues[0].submit(submitInfo, vk::Fence());
			m_transferQueues[0].waitIdle();
#endif

}
