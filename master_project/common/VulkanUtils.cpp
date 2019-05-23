
#include "pch.h"
#include "VulkanUtils.hpp"



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
