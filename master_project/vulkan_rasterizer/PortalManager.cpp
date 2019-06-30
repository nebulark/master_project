#include "pch.hpp"
#include "PortalManager.hpp"
#include "MeshDataManager.hpp"
#include "PushConstants.hpp"

void PortalManager::Add(const Portal& portal)
{
	m_portals.push_back(portal);
}

void PortalManager::DrawPortals(vk::CommandBuffer drawBuffer, MeshDataManager& meshDataManager,
	vk::PipelineLayout layout, int iterationElementIndex, std::vector<uint8_t> stencilRefs)
{
	drawBuffer.bindIndexBuffer(meshDataManager.GetIndexBuffer(), 0, MeshDataManager::IndexBufferIndexType);
	vk::DeviceSize vertexBufferOffset = 0;
	drawBuffer.bindVertexBuffers(0, meshDataManager.GetVertexBuffer(), vertexBufferOffset);

	const int firstChildIdx = NTree::GetChildElementIdx(m_portals.size() * 2, iterationElementIndex, 0);
	int currentChildIdx = firstChildIdx;

	for (const Portal& portal : m_portals)
	{
		const int a_childIndex = currentChildIdx;
		const int b_childIndex = a_childIndex + 1;
		currentChildIdx += 2;

		const MeshDataRef& portalMeshRef = meshDataManager.GetMeshes()[portal.meshIndex];

		PushConstant pushConstant = {};
		pushConstant.model = portal.a_transform.ToMat();
		pushConstant.cameraIdx = 0;
		pushConstant.portalStencilVal = stencilRefs[a_childIndex];
		pushConstant.debugColor = glm::vec4(0.5f, 0.f, 0.f, 1.f);

		drawBuffer.pushConstants<PushConstant>(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
		drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);

		pushConstant.model = portal.b_transform.ToMat();
		pushConstant.portalStencilVal = stencilRefs[b_childIndex];

		pushConstant.debugColor = glm::vec4(0.0f, 0.f, 0.5f, 1.f);

		drawBuffer.pushConstants<PushConstant>(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
		drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);
	}
}
