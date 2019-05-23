#pragma once
#include <gsl/gsl>

namespace VulkanUtils
{
	bool SupportsValidationLayers(gsl::span<const char*> layerNames);
}