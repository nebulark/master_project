
#include "pch.h"
#include "VulkanUtils.hpp"


namespace
{
	template<typename T>
	T ChooseBest(gsl::span<const T> available, gsl::span<const T> prefered)
	{
		assert(available.size() >= 1);
		assert(prefered.size() >= 1);

		constexpr size_t noPreferedAvailable = -1;
		size_t bestPreferedIndex = noPreferedAvailable;

		for (T currentAvailable : available)
		{
			if (currentAvailable == prefered[0])
			{
				return currentAvailable;
			}

			for (size_t preferedIdx = 1; preferedIdx < bestPreferedIndex; ++preferedIdx)
			{
				if (currentAvailable == prefered[preferedIdx])
				{
					bestPreferedIndex = preferedIdx;
					break;
				}
			}

		}

		if (noPreferedAvailable == bestPreferedIndex)
		{
			return available[0];
		}

		return prefered[bestPreferedIndex];
	}
}


bool VulkanUtils::SupportsValidationLayers(gsl::span<const char*> layerNames)
{
	const std::vector<vk::LayerProperties> availableLayerProperties = vk::enumerateInstanceLayerProperties();
	return std::all_of(layerNames.cbegin(), layerNames.cend(), [&availableLayerProperties](const char* layerName)
		{
			return std::any_of(availableLayerProperties.cbegin(), availableLayerProperties.cend(), [layerName](const vk::LayerProperties & availableLayerProperty)
				{
					return std::strcmp(availableLayerProperty.layerName, layerName) == 0;
				});
		});
}

vk::SurfaceFormatKHR VulkanUtils::ChooseSurfaceFormat(gsl::span<const vk::SurfaceFormatKHR> availableFormats, gsl::span<const vk::SurfaceFormatKHR> preferedFormat)
{
	assert(availableFormats.size() >= 1);
	assert(preferedFormat.size() >= 1);

	if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
	{
		return preferedFormat[0];
	}

	return ChooseBest(availableFormats, preferedFormat);
}

vk::PresentModeKHR VulkanUtils::ChoosePresentMode(
	gsl::span<const vk::PresentModeKHR> availableModes,
	gsl::span<const vk::PresentModeKHR> preferedModes)
{
	return ChooseBest(availableModes, preferedModes);	
}

vk::Extent2D VulkanUtils::ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D PreferedExtent)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	return vk::Extent2D(
		std::clamp(capabilities.currentExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	);	
}

uint32_t VulkanUtils::ChooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t preferedImageCount)
{
	if (capabilities.maxImageCount == 0)
	{
		return std::max(preferedImageCount, capabilities.minImageCount);
	}

	return std::clamp(preferedImageCount, capabilities.minImageCount, capabilities.maxImageCount);
}
