#include "pch.hpp"
#include "PortalManager.hpp"
#include "MeshDataManager.hpp"
#include "PushConstants.hpp"
#include "NTree.hpp"

void PortalManager::Add(const Portal& portal)
{
	m_portals.push_back(portal);
}

namespace {
	const glm::vec4 debugColors[] =
	{

		glm::vec4(0.6f, 0.2f, 0.2f, 1.f),
		glm::vec4(0.2f, 0.6f, 0.2f, 1.f),
		glm::vec4(0.2f, 0.2f, 0.6f, 1.f),

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
	vk::PipelineLayout layout, int iterationElementIndex, int numBitsToShiftStencil, int maxVisiblePortalCount, uint8_t stencilRef)
{
	const uint32_t actualPortalCount = GetSizeUint32(m_portals) * 2;

	drawBuffer.bindIndexBuffer(meshDataManager.GetIndexBuffer(), 0, MeshDataManager::IndexBufferIndexType);
	vk::DeviceSize vertexBufferOffset = 0;
	drawBuffer.bindVertexBuffers(0, meshDataManager.GetVertexBuffer(), vertexBufferOffset);

	const int firstChildIdx = NTree::GetChildElementIdx(actualPortalCount, iterationElementIndex, 0);

	for (int i = 0; i < m_portals.size(); ++i)
	{
		const int a_childNum = i * 2;
		const int b_childNum = a_childNum + 1;

		const MeshDataRef& portalMeshRef = meshDataManager.GetMeshes()[m_portals[i].meshIndex];

		PushConstant pushConstant = {};
		pushConstant.cameraIdx = iterationElementIndex;
		pushConstant.layerStencilVal = stencilRef;
		pushConstant.firstHelperIndex = firstChildIdx;
		pushConstant.firstCameraIndicesIndex = firstChildIdx;
		pushConstant.maxVisiblePortalCountForRecursion = maxVisiblePortalCount;
		pushConstant.numOfBitsToShiftChildStencilVal = numBitsToShiftStencil;

		
		pushConstant.debugColor = debugColors[i % std::size(debugColors)];

		{
			// comment this when ready to calc portal val in shader
			pushConstant.layerStencilVal = stencilRef | ((a_childNum + 1) << numBitsToShiftStencil);


			pushConstant.portalCameraIndex = a_childNum + firstChildIdx;

			pushConstant.currentHelperIndex = a_childNum + firstChildIdx;
			
			pushConstant.model = m_portals[i].a_transform.ToMat();
			drawBuffer.pushConstants<PushConstant>(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
			drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);
		}

		drawBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags{}, {}, {}, {});

		{
			pushConstant.portalCameraIndex = b_childNum + firstChildIdx;

			// comment this when ready to calc portal val in shader
			pushConstant.layerStencilVal = stencilRef | ((b_childNum + 1) << numBitsToShiftStencil);

			pushConstant.currentHelperIndex = b_childNum + firstChildIdx;

			pushConstant.model = m_portals[i].b_transform.ToMat();
			drawBuffer.pushConstants<PushConstant>(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
			drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);	
			
		
		}

	drawBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags{}, {}, {}, {});

	}
}

void PortalManager::CreateCameraTransforms(Transform cameraTransform, int maxRecursionCount, gsl::span<Transform> outTransforms) const
{
	// we build an NTree, with a child for each portal connections (portals are two sided so we have to connection per element in portals)
	// layer 0  of the NTree has 1 element
	// layer 1 has portalCount elements
	// layer 2 has portalCount^2 elements
	// ....

	// each children corresponds to the product between the parent matrix and the portals translation matrix (a_to_b / b_to_a)

	// each portal struct actually defines two portals
	// this value will we uses a N for the NTree

	const int portalCount = m_portals.size() * 2;
	const uint32_t matrixCount = NTree::CalcTotalElements(portalCount, maxRecursionCount + 1);
	assert(outTransforms.size() >= matrixCount);

	outTransforms[0] = cameraTransform;
	for (int cameraTreeLayer = 1; cameraTreeLayer <= maxRecursionCount; ++cameraTreeLayer)
	{
		const uint32_t previousLayerStartIndex = NTree::CalcFirstLayerIndex(portalCount, cameraTreeLayer - 1);
		const uint32_t layerStartIndex = NTree::CalcFirstLayerIndex(portalCount, cameraTreeLayer);

		// for each parent we iterate over all portals
		for (uint32_t parentIdx = previousLayerStartIndex; parentIdx < layerStartIndex; ++parentIdx)
		{
			// we are iterating over 2 elements at once, so the actual portals are 2*i and 2*i +1!
			for (uint32_t i = 0; i < m_portals.size(); ++i)
			{
				const uint32_t childIdx0 = NTree::GetChildElementIdx(portalCount, parentIdx, 2 * i);
				const uint32_t childIdx1 = childIdx0 + 1;

				// validate that we don't got out of range
				assert(childIdx1 < matrixCount);

				outTransforms[childIdx0] = m_portals[i].a_to_b * outTransforms[parentIdx];
				outTransforms[childIdx1] = m_portals[i].b_to_a * outTransforms[parentIdx];
			}
		}
	}

	// TODO: could be improved by keeping a parent and a child index and incrementing those as needed instead of recalculating all the time?
}

int PortalManager::GetCameraBufferElementCount(int maxRecursionCount) const
{
	const int portalCount = m_portals.size() * 2;
	return GetCameraBufferElementCount(maxRecursionCount, portalCount);
}

int PortalManager::GetCameraBufferElementCount(int maxRecursionCount, int portalCount)
{
	const int cameraBufferElementCount = NTree::CalcTotalElements(portalCount, maxRecursionCount + 1);
	return cameraBufferElementCount;
}

int PortalManager::GetPortalIndexHelperElementCount(int maxRecursionCount, int portalCount)
{
	return GetCameraBufferElementCount(maxRecursionCount, portalCount);
}

int PortalManager::GetPortalIndexHelperElementCount(int maxRecursionCount)
{
	return GetPortalIndexHelperElementCount(maxRecursionCount, GetPortalCount());
}

