#pragma once
#include <string>
#include <vulkan/vulkan.hpp>


struct MeshDataRef
{
	std::string meshName;
	vk::DeviceSize indexBufferOffset;
	uint32_t indexCount;
};

