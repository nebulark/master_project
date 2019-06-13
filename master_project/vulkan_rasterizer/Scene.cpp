#include "pch.hpp"
#include "Scene.hpp"

Scene::Scene(VmaAllocator allocator)
	: m_drawIndexedIndirectBuffer()
	, m_drawIndexedIndirectBufferElementCount(0)
	, m_allocator(allocator)
{

	// TODO: arbitrary values, benchmark what we really need
	constexpr int MaxElements = 20;


	VmaAllocationCreateInfo vmaAllocInfo_gpuOnly = {};
	vmaAllocInfo_gpuOnly.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

	{
		m_drawIndexedIndirectBufferMaxElements = MaxElements;
		const vk::BufferCreateInfo vertexBufferCreateInfo = vk::BufferCreateInfo{}
			.setUsage(vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eTransferDst)
			.setSize(m_drawIndexedIndirectBufferMaxElements * sizeof(vk::DrawIndexedIndirectCommand));

		m_drawIndexedIndirectBuffer = UniqueVmaBuffer(allocator, vertexBufferCreateInfo, vmaAllocInfo_gpuOnly);
	}


}