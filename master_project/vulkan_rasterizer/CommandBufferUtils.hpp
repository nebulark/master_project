#pragma once
#include <vulkan/vulkan.hpp>
namespace CbUtils
{

	vk::UniqueCommandBuffer AllocateSingle(vk::Device device, vk::CommandPool pool);

}
