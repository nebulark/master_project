#include "pch.hpp"
#include "CommandBufferUtils.hpp"

vk::UniqueCommandBuffer CbUtils::AllocateSingle(vk::Device device, vk::CommandPool pool)
{
	vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo{}
		.setCommandBufferCount(1)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandPool(pool);

	vk::CommandBuffer cb_temp;
	device.allocateCommandBuffers(&allocInfo, &cb_temp);

	vk::PoolFree<vk::Device, vk::CommandPool, vk::DispatchLoaderStatic> deleter(
		device, allocInfo.commandPool, vk::DispatchLoaderStatic());

	return vk::UniqueCommandBuffer(cb_temp, deleter);
}
