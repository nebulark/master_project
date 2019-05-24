#pragma once
#include <gsl/gsl>
#include <vulkan/vulkan.hpp>

namespace VulkanUtils
{
	bool SupportsValidationLayers(gsl::span<const char*> layerNames);

	vk::SurfaceFormatKHR ChooseSurfaceFormat(
		gsl::span<const vk::SurfaceFormatKHR> availableFormats,
		gsl::span<const vk::SurfaceFormatKHR> preferedFormat);

	vk::PresentModeKHR ChoosePresentMode(
		gsl::span<const vk::PresentModeKHR> availableModes,
		gsl::span<const vk::PresentModeKHR> preferedModes);

	vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D referedExtent);

	uint32_t ChooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t preferedImageCount);

}