#pragma once
#include <vulkan/vulkan.hpp>
#include "SDL.h"
#include "VmaRAII.hpp"
#include "Swapchain.hpp"
#include "common/VulkanDevice.hpp"

#include "VmaAllocationsPool.hpp"
#include "MeshDataManager.hpp"
#include "Scene.hpp"
#include "Portal.hpp"
#include "UniqueVmaObject.hpp"

class Camera;

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
	vk::Queue m_graphicsPresentQueues;

	vk::UniqueShaderModule m_vertShaderModule;
	vk::UniqueShaderModule m_fragShaderModule;

	vk::UniqueShaderModule m_vertShaderModule_portal;
	vk::UniqueShaderModule m_fragShaderModule_portal;

	Swapchain m_swapchain;
	vk::Format m_depthFormat;
	vk::Image m_depthBuffer;
	std::vector<vk::UniqueFramebuffer> m_framebuffer;

	vk::UniqueRenderPass m_colorDepthRenderPass;

	vk::UniqueDescriptorPool m_descriptorPool;

	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_texture;
	vk::DescriptorSet m_descriptorSet_texture;

	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_ubo;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_cameraMat;
	std::array<vk::DescriptorSet , MaxInFlightFrames> m_descriptorSet_ubo;
	std::array<vk::DescriptorSet , MaxInFlightFrames> m_descriptorSet_cameratMat;
	std::array<UniqueVmaBuffer, MaxInFlightFrames> m_ubo_buffer;
	std::array<UniqueVmaBuffer, MaxInFlightFrames> m_cameratMat_buffer;

	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_renderedDepth;
	std::array<UniqueVmaImage,2> m_image_renderedDepth;
	std::array<vk::UniqueImageView,2> m_imageview_renderedDepth;
	std::array<vk::DescriptorSet,2> m_descriptorSet_renderedDepth;

	vk::UniquePipelineLayout m_pipelineLayout;

	std::array<vk::UniqueCommandPool, MaxInFlightFrames> m_graphicsPresentCommandPools;
	std::array<vk::UniqueCommandBuffer, MaxInFlightFrames> m_graphicsPresentBuffer;

	std::array<vk::UniqueFence, MaxInFlightFrames> m_frameFence;
	std::array<vk::UniqueSemaphore, MaxInFlightFrames> m_imageAvailableSem;
	std::array<vk::UniqueSemaphore, MaxInFlightFrames> m_renderFinishedSem;
	int m_currentframe = 0;

	vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> m_graphicsPipeline_scene_initial;
	vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> m_graphicsPipeline_scene_subsequent;
	vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> m_graphicsPipeline_portals_initial;
	vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> m_graphicsPipeline_portals_subesquent;
	vk::UniqueSampler m_textureSampler;
	vk::Image m_textureImage;
	vk::UniqueImageView m_textureImageView;
	vk::UniqueImageView m_depthBufferView;
	std::unique_ptr<MeshDataManager> m_meshData;
	std::unique_ptr<Scene> m_scene;
	Portal m_portal;
};
