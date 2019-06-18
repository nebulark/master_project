#pragma once
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <vector>

struct Swapchain
{
	static Swapchain Create(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface, vk::Extent2D extent);
	
	vk::UniqueSwapchainKHR swapchain;
	std::vector<vk::Image> images;
	std::vector<vk::UniqueImageView> imageViews;
	vk::Extent2D extent;

	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;
};

