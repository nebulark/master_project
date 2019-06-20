#include "pch.hpp"
#include "Scene.hpp"
#include "MeshDataManager.hpp"
#include "PushConstants.hpp"

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

void Scene::Add(int MeshIdx, const glm::mat4& modelmat)
{
	m_objects.push_back(SceneObject{ modelmat, MeshIdx });
}

void Scene::Draw(MeshDataManager& meshdataManager, vk::PipelineLayout pipelineLayout, vk::CommandBuffer drawCommandBuffer) const
{
	drawCommandBuffer.bindIndexBuffer(meshdataManager.GetIndexBuffer(), 0, MeshDataManager::IndexBufferIndexType);
	vk::DeviceSize vertexBufferOffset = 0;
	drawCommandBuffer.bindVertexBuffers(0, meshdataManager.GetVertexBuffer(), vertexBufferOffset);

	gsl::span<const MeshDataRef> meshDataRefs = meshdataManager.GetMeshes();

	for (const SceneObject& object : m_objects)
	{

		PushConstant_ModelMat pushConstant = {};
		pushConstant.model = object.modelMat;

		drawCommandBuffer.pushConstants<PushConstant_ModelMat>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, pushConstant);
		const MeshDataRef& portalMeshRef = meshDataRefs[object.meshIdx];
		drawCommandBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);
	}
}
