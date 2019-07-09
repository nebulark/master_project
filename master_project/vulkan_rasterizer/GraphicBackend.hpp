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
#include "PortalManager.hpp"
#include "NTree.hpp"
#include "StencilRefTree.hpp"

class Camera;

class GraphicsBackend
{
public:
	void Init(SDL_Window* window);
	void Render(const Camera& camera);
	void WaitIdle() { m_device->waitIdle(); }
private:
	static constexpr int MaxInFlightFrames = 2;	
	static constexpr int maxVisiblePortalsForRecursion[] = {4,3,2};
	static constexpr int recursionCount = std::size(maxVisiblePortalsForRecursion);

	vk::UniqueInstance m_vkInstance;
	vk::PhysicalDevice m_physicalDevice;
	vk::UniqueDevice m_device;
	VmaRAII::UniqueVmaAllocator m_allocator;

	vk::UniqueDebugUtilsMessengerEXT m_debugUtilsMessenger;

	vk::UniqueSurfaceKHR m_surface;

	VulkanDevice::QueueResult m_graphicsPresentQueueInfo;
	vk::Queue m_graphicsPresentQueues;

	vk::UniqueShaderModule m_vertShaderModule;
	vk::UniqueShaderModule m_fragShaderModule;
	vk::UniqueShaderModule m_fragShaderModule_subsequent;

	vk::UniqueShaderModule m_vertShaderModule_portal;
	vk::UniqueShaderModule m_fragShaderModule_portal;
	vk::UniqueShaderModule m_fragShaderModule_portal_subsequent;

	Swapchain m_swapchain;
	vk::Format m_depthFormat;
	UniqueVmaImage m_depthBuffer;
	vk::UniqueImageView m_depthBufferView;

	std::vector<vk::UniqueFramebuffer> m_framebuffer;

	vk::UniqueRenderPass m_portalRenderPass;

	vk::UniqueDescriptorPool m_descriptorPool;

	vk::UniqueSampler m_textureSampler;
	UniqueVmaImage m_textureImage;
	vk::UniqueImageView m_textureImageView;

	std::array<UniqueVmaBuffer, MaxInFlightFrames> m_ubo_buffer;
	std::array<UniqueVmaBuffer, MaxInFlightFrames> m_cameratMat_buffer;

	// Stores indices to access the camera mat buffer
	std::array<UniqueVmaBuffer, MaxInFlightFrames> m_cameraIndexBuffer;

	// only used by portal rendering, to calc its index, so it can write into the correct location of cameraMatIndexBuffer
	std::array<UniqueVmaBuffer, MaxInFlightFrames> m_portalIndexHelperBuffer;

	std::array<UniqueVmaImage, 2> m_image_renderedDepth;
	std::array<vk::UniqueImageView, 2> m_imageview_renderedDepth;


	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_texture;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_ubo;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_cameraMat;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_cameraIndices;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_portalIndexHelper;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_renderedDepth;

	vk::DescriptorSet m_descriptorSet_texture;
	std::array<vk::DescriptorSet, MaxInFlightFrames> m_descriptorSet_ubo;
	std::array<vk::DescriptorSet, MaxInFlightFrames> m_descriptorSet_cameratMat;
	std::array<vk::DescriptorSet, MaxInFlightFrames> m_descriptorSet_cameraIndices;
	std::array<vk::DescriptorSet, MaxInFlightFrames> m_descriptorSet_portalIndexHelper;
	std::array<vk::DescriptorSet, 2> m_descriptorSet_renderedDepth;

	vk::UniquePipelineLayout m_pipelineLayout;

	std::array<vk::UniqueCommandPool, MaxInFlightFrames> m_graphicsPresentCommandPools;
	std::array<vk::UniqueCommandBuffer, MaxInFlightFrames> m_graphicsPresentBuffer;

	std::array<vk::UniqueFence, MaxInFlightFrames> m_frameFence;
	std::array<vk::UniqueSemaphore, MaxInFlightFrames> m_imageAvailableSem;
	std::array<vk::UniqueSemaphore, MaxInFlightFrames> m_renderFinishedSem;
	int m_currentframe = 0;

	std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> m_graphicPipelines;
	StencilRefTree m_stencilRefTree;

	std::unique_ptr<MeshDataManager> m_meshData;
	std::unique_ptr<Scene> m_scene;
	PortalManager m_portalManager;
};
