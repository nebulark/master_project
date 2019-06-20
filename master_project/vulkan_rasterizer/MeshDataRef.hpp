#pragma once
#include <string>
#include <vulkan/vulkan.hpp>


struct MeshDataRef
{
	std::string meshName;
	uint32_t firstIndex;
	uint32_t indexCount;
};

