#pragma once
#include <gsl/gsl>
#include <vulkan/vulkan.hpp>

namespace VulkanUtils
{
	template<typename Uint>
	Uint AlignDown(Uint integer, Uint alignment)
	{
		static_assert(std::is_unsigned_v<Uint>);
		const Uint alignMask = alignment - 1;

		// sets everything to zero, defined by align mask
		Uint result = integer & ~alignMask;
	
		assert(result <= integer && (result % alignment == 0));
		return result;
	}

	template<typename Uint>
	Uint AlignUp(Uint integer, Uint alignment)
	{
		static_assert(std::is_unsigned_v<Uint>);
		Uint result = AlignDown(integer + alignment - 1, alignment);
		assert(result >= integer && (result % alignment == 0));
		return result;
	}

	bool SupportsValidationLayers(gsl::span<const char*> layerNames);

	vk::SurfaceFormatKHR ChooseSurfaceFormat(
		gsl::span<const vk::SurfaceFormatKHR> availableFormats,
		gsl::span<const vk::SurfaceFormatKHR> preferedFormat);

	vk::PresentModeKHR ChoosePresentMode(
		gsl::span<const vk::PresentModeKHR> availableModes,
		gsl::span<const vk::PresentModeKHR> preferedModes);

	inline bool HasStencilComponent(vk::Format format)
	{ return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint; }

	vk::Format ChooseFormat(vk::PhysicalDevice physicalDevice, gsl::span<const vk::Format> preferedFormats, vk::ImageTiling tiling, vk::FormatFeatureFlags featureFlags);
	vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D referedExtent);

	uint32_t ChooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t preferedImageCount);

}