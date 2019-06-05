#pragma once
#include <vulkan/vulkan.hpp>
#include "SDL.h"
#include "VmaRAII.hpp"
#include "Swapchain.hpp"
#include "common/VulkanDevice.hpp"

#include "VmaAllocationsPool.hpp"

class Camera;

struct ModelBuffer
{
	static const vk::BufferUsageFlags usage;

	vk::Buffer buffer;
	uint32_t indexOffset;
	uint32_t indexCount;
	uint32_t totalBufferSize;
	vk::IndexType indexType;
	uint32_t vertexOffset;
};
class GraphicsBackend
{
public:
	void Init(SDL_Window* window);
	void Render(const Camera& camera);
	void WaitIdle() { m_device->waitIdle(); }
private:
	static constexpr int MaxInFlightFrames = 2;
	vk::UniqueInstance m_vkInstance;
	vk::PhysicalDevice m_physicalDevice;
	vk::UniqueDevice m_device;
	VmaRAII::UniqueVmaAllocator m_allocator;
	VmaBufferPool m_BufferPool;
	VmaImagePool m_ImagePool;

	vk::UniqueDebugUtilsMessengerEXT m_debugUtilsMessenger;

	vk::UniqueSurfaceKHR m_surface;

	VulkanDevice::QueueResult m_graphicsPresentQueueInfo;
	std::array<vk::Queue, 1> m_graphicsPresentQueues;

	VulkanDevice::QueueResult m_transferQueueInfo;
	std::array<vk::Queue, 1> m_transferQueues;
	vk::UniqueShaderModule m_vertShaderModule;
	vk::UniqueShaderModule m_fragShaderModule;

	Swapchain m_swapchain;
	vk::Format m_depthFormat;
	vk::Image m_depthBuffer;
	std::vector<vk::UniqueFramebuffer> m_framebuffer;

	vk::UniqueRenderPass m_colorDepthRenderPass;

	vk::UniqueDescriptorPool m_descriptorPool;

	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_texture;
	vk::DescriptorSet m_descriptorSet_texture;

	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_ubo;
	std::array<vk::DescriptorSet , MaxInFlightFrames> m_descriptorSet_ubo;
	std::array<vk::Buffer, MaxInFlightFrames> m_ubo_buffer;

	vk::UniquePipelineLayout m_pipelineLayout;

	std::array<vk::UniqueCommandPool, MaxInFlightFrames> m_graphicsPresentCommandPools;
	std::array<vk::UniqueCommandBuffer, MaxInFlightFrames> m_graphicsPresentBuffer;

	vk::UniqueCommandPool m_transferQueuePool;
	std::array<vk::UniqueFence, MaxInFlightFrames> m_frameFence;
	std::array<vk::UniqueSemaphore, MaxInFlightFrames> m_imageAvailableSem;
	std::array<vk::UniqueSemaphore, MaxInFlightFrames> m_renderFinishedSem;
	int m_currentframe = 0;

	vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> m_graphicsPipeline;
	vk::UniqueSampler m_textureSampler;
	ModelBuffer m_modelBuffer;
	vk::Image m_textureImage;
	vk::UniqueImageView m_textureImageView;
	vk::UniqueImageView m_depthBufferView;
};
