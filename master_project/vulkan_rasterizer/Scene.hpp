#pragma once
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>
#include <string>
#include "Vertex.hpp"
#include "common/VulkanUtils.hpp"
#include "UniqueVmaBuffer.hpp"
#include "MeshDataRef.hpp"

class Scene
{
public:
	Scene(VmaAllocator allocator);
private:
	
	UniqueVmaBuffer m_drawIndexedIndirectBuffer;
	int m_drawIndexedIndirectBufferElementCount;
	int m_drawIndexedIndirectBufferMaxElements;

	VmaAllocator m_allocator;
};
