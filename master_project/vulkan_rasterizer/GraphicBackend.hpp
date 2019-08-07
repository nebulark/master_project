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
#include "TriangleMesh.hpp"
#include "LineDrawer.hpp"

class Camera;

struct DrawOptions
{
	std::vector<Line> extraLines;
	int maxRecursion = std::numeric_limits<int>::max();
};

class GraphicsBackend
{
public:
	void Init(SDL_Window* window, Camera& camera);
	void Render(const Camera& camera, const DrawOptions&  drawoptions);
	void WaitIdle() { m_device->waitIdle(); }

	gsl::span<const TriangleMesh> GetTriangleMeshes() const { return m_triangleMeshes; }
	const PortalManager& GetPortalManager() const { return m_portalManager; }

	void SetMaxVisiblePortalsForRecursion(gsl::span<const int> visiblePortals) { m_maxVisiblePortalsForRecursion = visiblePortals; }
private:
	static constexpr int MaxInFlightFrames = 2;	
	static constexpr int maxPortalCount = 12;
	static constexpr int worstMaxVisiblePortalsForRecursion[] = 
		{ maxPortalCount, maxPortalCount, maxPortalCount, maxPortalCount, maxPortalCount };
	static constexpr int worstRecursionCount = gsl::narrow<int>(std::size(worstMaxVisiblePortalsForRecursion));


	static constexpr int cameraMatricesMaxCount = NTree::CalcTotalElements(maxPortalCount, worstRecursionCount + 1);
	static_assert(cameraMatricesMaxCount <= 3257437);

	gsl::span<const int> m_maxVisiblePortalsForRecursion;

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

	vk::UniqueShaderModule m_vertShaderModule_lines;
	vk::UniqueShaderModule m_fragShaderModule_lines;
	vk::UniqueShaderModule m_fragShaderModule_lines_subsequent;

	vk::UniqueShaderModule m_vertShaderModule_portal;
	vk::UniqueShaderModule m_fragShaderModule_portal;
	vk::UniqueShaderModule m_fragShaderModule_portal_subsequent;

	Swapchain m_swapchain;
	vk::Format m_depthStencilFormat;
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

	std::array<UniqueVmaImage, 2> m_image_renderedStencil;
	std::array<vk::UniqueImageView, 2> m_imageview_renderedStencil;

	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_texture;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_ubo;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_cameraMat;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_cameraIndices;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_portalIndexHelper;
	vk::UniqueDescriptorSetLayout m_descriptorSetLayout_rendered;

	vk::DescriptorSet m_descriptorSet_texture;
	std::array<vk::DescriptorSet, MaxInFlightFrames> m_descriptorSet_ubo;
	std::array<vk::DescriptorSet, MaxInFlightFrames> m_descriptorSet_cameratMat;
	std::array<vk::DescriptorSet, MaxInFlightFrames> m_descriptorSet_cameraIndices;
	std::array<vk::DescriptorSet, MaxInFlightFrames> m_descriptorSet_portalIndexHelper;
	std::array<vk::DescriptorSet, 2> m_descriptorSet_rendered;

	

	vk::UniquePipelineLayout m_pipelineLayout_portal;
	vk::UniquePipelineLayout m_pipelineLayout_scene;
	vk::UniquePipelineLayout m_pipelineLayout_lines;

	std::array<vk::UniqueCommandPool, MaxInFlightFrames> m_graphicsPresentCommandPools;
	std::array<vk::UniqueCommandBuffer, MaxInFlightFrames> m_graphicsPresentBuffer;

	std::array<vk::UniqueFence, MaxInFlightFrames> m_frameFence;
	std::array<vk::UniqueSemaphore, MaxInFlightFrames> m_imageAvailableSem;
	std::array<vk::UniqueSemaphore, MaxInFlightFrames> m_renderFinishedSem;
	int m_currentframe = 0;


	struct ScenePassPipelines
	{

		std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> scene;
		std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> line;
	};

	struct PortalPassPipelines
	{
		std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> portal;
	};

	struct Pipelines
	{

		ScenePassPipelines scenePass;
		PortalPassPipelines portalPass;
	};

	Pipelines m_pipelines;


	std::unique_ptr<MeshDataManager> m_meshData;
	std::vector<TriangleMesh> m_triangleMeshes;
	std::unique_ptr<Scene> m_scene;
	PortalManager m_portalManager;
	std::vector<Line> m_portalAABBLines;
};
