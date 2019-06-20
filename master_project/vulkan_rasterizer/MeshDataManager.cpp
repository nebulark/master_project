#include "pch.hpp"
#include "MeshDataManager.hpp"
#include "GetSizeUint32.hpp"
#include "UniqueVmaObject.hpp"
#include "UniqueVmaMemoryMap.hpp"
#include "CommandBufferUtils.hpp"


MeshDataManager::MeshDataManager(VmaAllocator allocator)
	: m_vertexBufferElementCount(0)
	, m_indexBufferElementCount(0)
	, m_allocator(allocator)
{

	// TODO: arbitrary values, benchmark what we really need
	constexpr int MaxVertices = 1'000'000;
	constexpr int MaxIndices = 6 * MaxVertices;


	VmaAllocationCreateInfo vmaAllocInfo_gpuOnly = {};
	vmaAllocInfo_gpuOnly.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

	{
		m_vertexBufferMaxElements = MaxVertices;
		const vk::BufferCreateInfo vertexBufferCreateInfo = vk::BufferCreateInfo{}
			.setUsage(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst)
			.setSize(m_vertexBufferMaxElements * sizeof(VertexType));

		m_vertexBuffer = UniqueVmaBuffer(allocator, vertexBufferCreateInfo, vmaAllocInfo_gpuOnly);
	}

	{
		m_indexBufferMaxElements = MaxIndices;
		const vk::BufferCreateInfo indexBufferCreateInfo = vk::BufferCreateInfo{}
			.setUsage(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst)
			.setSize(m_indexBufferMaxElements * sizeof(IndexType));

		m_indexBuffer = UniqueVmaBuffer(allocator, indexBufferCreateInfo, vmaAllocInfo_gpuOnly);
	}
}


void MeshDataManager::LoadObjs(gsl::span<const char* const> objFileNames,
	vk::Device device, vk::CommandPool transferPool, vk::Queue transferQueue)
{

	const int initialIndexElementCount = m_indexBufferElementCount;
	const int initialVertexElementCount = m_vertexBufferElementCount;
	const vk::DeviceSize indexBufferOffset = m_indexBufferElementCount * sizeof(IndexType);
	const vk::DeviceSize vertexBufferOffset = m_vertexBufferElementCount * sizeof(VertexType);

	std::vector<VertexType> vertices;
	std::vector<IndexType> indices;

	for (const char* filename : objFileNames)
	{

		// we could also use the member variable, but we would need to keep it up to date
		const int currentIndexBufferElementCount = initialIndexElementCount + gsl::narrow<int>(indices.size());
		MeshDataRef staticSceneMesh;
		staticSceneMesh.meshName = filename;
		staticSceneMesh.firstIndex = currentIndexBufferElementCount;

		Vertex::LoadObjWithIndices_append(filename, vertices, indices);
		staticSceneMesh.indexCount = gsl::narrow<uint32_t>(indices.size() - currentIndexBufferElementCount);
		
		m_meshes.push_back(std::move(staticSceneMesh));
	}

	assert(m_vertexBufferElementCount + vertices.size() <= m_vertexBufferMaxElements);
	assert(m_indexBufferElementCount + indices.size() <= m_indexBufferMaxElements);
	// Copy Buffer to GPU
	{
		const uint32_t indexBufferSizeBytes = sizeof(indices[0]) * GetSizeUint32(indices);
		const uint32_t vertexBufferSizeBytes = sizeof(vertices[0]) * GetSizeUint32(vertices);
		const uint32_t stageBufferBegin = 0;
		const uint32_t stageBufferIndicesBegin = stageBufferBegin;
		const uint32_t stageBufferIndicesEnd = stageBufferIndicesBegin + indexBufferSizeBytes;

		const uint32_t stageBufferVertexBegin = VulkanUtils::AlignUp<uint32_t>(indexBufferSizeBytes, alignof(Vertex));
		const uint32_t stageBufferVertexEnd = stageBufferVertexBegin + vertexBufferSizeBytes;

		const uint32_t stageBufferSize = stageBufferVertexEnd;


		const vk::BufferCreateInfo stagingBufferCreateInfo = vk::BufferCreateInfo{}
			.setSize(stageBufferSize)
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
			.setSharingMode(vk::SharingMode::eExclusive);


		VmaAllocationCreateInfo vmaAllocCreateInfo = {};
		vmaAllocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
		UniqueVmaBuffer stagingBuffer(m_allocator, stagingBufferCreateInfo, vmaAllocCreateInfo);

		{
			UniqueVmaMemoryMap memoryMap(stagingBuffer.GetAllocator(), stagingBuffer.GetAllocation());
			std::memcpy(memoryMap.GetMappedMemoryPtr() + stageBufferIndicesBegin, indices.data(), indexBufferSizeBytes);
			std::memcpy(memoryMap.GetMappedMemoryPtr() + stageBufferVertexBegin, vertices.data(), vertexBufferSizeBytes);
		}

		vk::UniqueCommandBuffer copyCommandBuffer = CbUtils::AllocateSingle(device, transferPool);
		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		copyCommandBuffer->begin(beginInfo);

		const vk::DeviceSize indicesByteSize = indices.size() * sizeof(IndexType);
		const vk::DeviceSize verticesByteSize = vertices.size() * sizeof(Vertex);

		
		{
			const vk::BufferCopy bufferCopyIndices(stageBufferIndicesBegin, indexBufferOffset, indicesByteSize);
			copyCommandBuffer->copyBuffer(stagingBuffer.Get(), m_indexBuffer.Get(), bufferCopyIndices);
		}
		{
			const vk::BufferCopy bufferCopyVertices(stageBufferVertexBegin, vertexBufferOffset, verticesByteSize);
			copyCommandBuffer->copyBuffer(stagingBuffer.Get(), m_vertexBuffer.Get(), bufferCopyVertices);
		}

	
		copyCommandBuffer->end();

		vk::SubmitInfo submitInfo = vk::SubmitInfo{}
			.setCommandBufferCount(1)
			.setPCommandBuffers(&(copyCommandBuffer.get()));

		transferQueue.submit(submitInfo, vk::Fence{});

		// for now we just wait for finish
		// could be improved to not block, but we would need to store the command buffer and the staging buffer until the operation is over
		transferQueue.waitIdle();

	}
	// increment Elemement count to match our new size
	m_indexBufferElementCount += GetSizeUint32(indices);
	m_vertexBufferElementCount += GetSizeUint32(vertices);
}
