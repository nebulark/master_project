#pragma once

#include <vulkan/vulkan.hpp>

#include "common/Deleters.hpp"
#include "common/VulkanDevice.hpp"

struct SimpleBuffer
{
	vk::UniqueDeviceMemory bufferMemory;
	vk::UniqueBuffer buffer;
	uint32_t size;
};


class Application
{
public:
	Application();

	void CreateSwapchain();

	void ReCreateSwapChain();

	bool Update();

	Application(const Application&) = delete;
	Application(Application&&) = delete;

	Application& operator=(const Application&) = delete;
	Application& operator=(Application&&) = delete;


private:
	WindowPtr m_sdlWindow;
	vk::UniqueInstance m_vkInstance;

	vk::UniqueDebugUtilsMessengerEXT m_debugUtilsMessenger;

	vk::UniqueSurfaceKHR m_surface;

	vk::PhysicalDevice m_physicalDevice;
	vk::UniqueDevice m_logicalDevice;

	std::vector<VulkanDevice::QueueResult> m_deviceQueueInfo;
	vk::Queue m_graphicsPresentQueue;
	vk::Queue m_transferQueue;
	          
	vk::UniqueHandle<vk::SwapchainKHR, vk::DispatchLoaderStatic> m_swapChain;

	vk::UniqueHandle<vk::RenderPass, vk::DispatchLoaderStatic> m_renderpass;

	vk::UniqueShaderModule m_vertShaderModule;
	vk::UniqueShaderModule m_fragShaderModule;

	vk::UniqueHandle<vk::PipelineLayout, vk::DispatchLoaderStatic> m_pipelineLayout;
	vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> m_graphicsPipeline;

	// one per swapchain image
	std::vector<vk::UniqueImageView> m_swapchainImageViews;

	// one for each swapchain image view
	std::vector<vk::UniqueFramebuffer> m_swapChainFramebuffers;

	// shares memory with transfer and graphicsPresent queue
	SimpleBuffer m_vertexBuffer;

	SimpleBuffer m_vertexBuffer_quad;
	SimpleBuffer m_indexBuffer_quad;

	vk::UniqueCommandPool m_graphcisPresentQueueCommandPool;
	vk::UniqueCommandPool m_transferQueueCommandPool;


	// one for each framebuffer
	std::vector<vk::UniqueCommandBuffer> m_commandBuffers;

	static constexpr int maxFramesInFlight = 2;

	std::array< vk::UniqueSemaphore, maxFramesInFlight> m_imageAvailableSemaphores;
	std::array< vk::UniqueSemaphore, maxFramesInFlight> m_renderFinishedSemaphores;
	std::array< vk::UniqueFence, maxFramesInFlight> m_inFlightFences;
	int m_currentFrame;
};