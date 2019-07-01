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
	const uint32_t actualPortalCount = GetSizeUint32(m_portals) * 2;

	drawBuffer.bindIndexBuffer(meshDataManager.GetIndexBuffer(), 0, MeshDataManager::IndexBufferIndexType);
	vk::DeviceSize vertexBufferOffset = 0;
	drawBuffer.bindVertexBuffers(0, meshDataManager.GetVertexBuffer(), vertexBufferOffset);

	const int firstChildIdx = NTree::GetChildElementIdx(actualPortalCount, iterationElementIndex, 0);

	
	const bool isLastPortalIteration = firstChildIdx >= stencilRefs.size();

	for (int i = 0; i < m_portals.size(); ++i)
	{
		const int a_childNum = i * 2;
		const int b_childNum = a_childNum + 1;


		const MeshDataRef& portalMeshRef = meshDataManager.GetMeshes()[m_portals[i].meshIndex];

		PushConstant pushConstant = {};
		pushConstant.cameraIdx = iterationElementIndex;

		pushConstant.debugColor =  isLastPortalIteration 
			? glm::vec4(1.f,1.f,1.f,1.f)
			: debugColors[iterationElementIndex % std::size(debugColors)];

		{
			pushConstant.portalStencilVal = isLastPortalIteration
				? stencilRefs[iterationElementIndex]
				: stencilRefs[a_childNum + firstChildIdx]
				;

			pushConstant.model = m_portals[i].a_transform.ToMat();
			drawBuffer.pushConstants<PushConstant>(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
			drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);
		}

		drawBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags{}, {}, {}, {});

		{
			pushConstant.portalStencilVal = isLastPortalIteration
				? stencilRefs[iterationElementIndex]
				: stencilRefs[b_childNum + firstChildIdx]
				;

			pushConstant.model = m_portals[i].b_transform.ToMat();
			drawBuffer.pushConstants<PushConstant>(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
			drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);	
			
		
		}

	drawBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags{}, {}, {}, {});

	}
}
