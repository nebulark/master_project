#include "pch.hpp"
#include "PortalManager.hpp"
#include "MeshDataManager.hpp"
#include "PushConstants.hpp"

void PortalManager::Add(const Portal& portal)
{
	m_portals.push_back(portal);
}

namespace {
	const glm::vec4 debugColors[] =
	{

		glm::vec4(1.f, 0.f, 0.f, 1.f),
		glm::vec4(0.f, 1.f, 0.f, 1.f),
		glm::vec4(0.f, 0.f, 1.f, 1.f),



		glm::vec4(0.75f, 0.25f, 0.f, 1.f),
		glm::vec4(0.5f, 0.5f, 0.f, 1.f),
		glm::vec4(0.25f, 0.75f, 0.f, 1.f),
		glm::vec4(0.f, 1.f, 0.f, 1.f),
		glm::vec4(0.f, 0.75f, 0.25f, 1.f),
		glm::vec4(0.f, 0.5f, 0.5f, 1.f),
		glm::vec4(0.f, 0.25f, 0.75f, 1.f),
		glm::vec4(0.f, 0.f, 1.f, 1.f),
		glm::vec4(0.25f, 0.f, 1.f, 0.75f),
		glm::vec4(0.5f, 0.f, 1.f, 0.5f),
		glm::vec4(0.75f, 0.f, 1.f, 0.25f),

	};

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
		pushConstant.cameraIdx = iterationElementIndex;

		pushConstant.debugColor = debugColors[iterationElementIndex % std::size(debugColors)];

		if (a_childIndex >= stencilRefs.size())
		{
			pushConstant.debugColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
		}

		pushConstant.model = portal.a_transform.ToMat();
		// if this is false this is the last portal rendering and we won't need to set stencil
		if (a_childIndex < stencilRefs.size())
		{
			pushConstant.portalStencilVal = stencilRefs[a_childIndex];
			//printf("portal %i a set stencil %i\n", iterationElementIndex, stencilRefs[a_childIndex]);
		}
		else
		{
			pushConstant.portalStencilVal = stencilRefs[iterationElementIndex];
		}

		drawBuffer.pushConstants<PushConstant>(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
		drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);

		pushConstant.model = portal.b_transform.ToMat();
		// if this is false this is the last portal rendering and we won't need to set stencil
		if (b_childIndex < stencilRefs.size())
		{
			pushConstant.portalStencilVal = stencilRefs[b_childIndex];
			//printf("portal %i b set stencil %i\n", iterationElementIndex, stencilRefs[b_childIndex]);
		}
		else
		{
			pushConstant.portalStencilVal = stencilRefs[iterationElementIndex];
		}


		drawBuffer.pushConstants<PushConstant>(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
		drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);
	}
}
