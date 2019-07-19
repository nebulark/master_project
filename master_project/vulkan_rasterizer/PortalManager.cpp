#include "pch.hpp"
#include "PortalManager.hpp"
#include "MeshDataManager.hpp"
#include "PushConstants.hpp"
#include "NTree.hpp"
#include "TriangleMesh.hpp"

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

void PortalManager::DrawPortals(const DrawPortalsInfo& info)
{
	const uint32_t actualPortalCount = GetSizeUint32(m_portals) * 2;
	assert(info.meshDataManager);
	MeshDataManager& meshDataManager = *info.meshDataManager;

	vk::CommandBuffer drawBuffer = info.drawBuffer;

	info.drawBuffer.bindIndexBuffer(meshDataManager.GetIndexBuffer(), 0, MeshDataManager::IndexBufferIndexType);


	vk::DeviceSize vertexBufferOffset = 0;
	drawBuffer.bindVertexBuffers(0, meshDataManager.GetVertexBuffer(), vertexBufferOffset);


	PushConstant_portal pushConstant = {};
	pushConstant.layerStartIndex = info.layerStartIndex;
	pushConstant.nextLayerStartIndex = info.nextLayerStartIndex;
	pushConstant.maxVisiblePortalCount = info.maxVisiblePortalCount;

	const int instanceCount = info.nextLayerStartIndex - info.layerStartIndex;

	const int32_t portalElementCount = gsl::narrow<int32_t>(m_portals.size());

	for (int32_t i = 0; i < portalElementCount; ++i)
	{
		const int32_t baseChildNum = i * 2;

		const MeshDataRef& portalMeshRef = meshDataManager.GetMeshes()[m_portals[i].meshIndex];

		for (auto endPoint = PortalEndpointIndex::First(); endPoint <= PortalEndpointIndex::Last(); ++endPoint)
		{
			const int32_t childNum = baseChildNum + gsl::narrow<int32_t>(endPoint.ToIndex());

			pushConstant.portalIndex = childNum;
			pushConstant.model = m_portals[i].transform[endPoint];
			pushConstant.debugColor = debugColors[(childNum + info.layerStartIndex) % std::size(debugColors)];
			drawBuffer.pushConstants<PushConstant_portal>(
				info.layout,
				vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0,
				pushConstant);
			
			drawBuffer.drawIndexed(portalMeshRef.indexCount, instanceCount, portalMeshRef.firstIndex, 0, 0);

			drawBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlags{}, {}, {}, {});

		}
	}
}

void PortalManager::CreateCameraMats(glm::mat4 cameraMat, int maxRecursionCount, gsl::span<glm::mat4> outCameraTransforms) const
{
	// we build an NTree, with a child for each portal connections (portals are two sided so we have to connection per element in portals)
	// layer 0  of the NTree has 1 element
	// layer 1 has portalCount elements
	// layer 2 has portalCount^2 elements
	// ....

	// each children corresponds to the product between the parent matrix and the portals translation matrix (a_to_b / b_to_a)

	// each portal struct actually defines two portals
	// this value will we uses a N for the NTree

	const uint32_t portalCount = gsl::narrow<uint32_t>(m_portals.size() * 2);
	const uint32_t matrixCount = NTree::CalcTotalElements(portalCount, maxRecursionCount + 1);
	assert(outCameraTransforms.size() >= matrixCount);

	outCameraTransforms[0] = cameraMat;
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

				outCameraTransforms[childIdx0] = m_portals[i].toOtherEndpoint[PortalEndpointIndex(PortalEndpoint::A)] * outCameraTransforms[parentIdx];
				outCameraTransforms[childIdx1] = m_portals[i].toOtherEndpoint[PortalEndpointIndex(PortalEndpoint::B)] * outCameraTransforms[parentIdx];
			}
		}
	}

}

int PortalManager::GetCameraBufferElementCount(int maxRecursionCount) const
{
	const int portalCount = gsl::narrow<int>(m_portals.size() * 2);
	return GetCameraBufferElementCount(maxRecursionCount, portalCount);
}

int PortalManager::GetCameraBufferElementCount(int maxRecursionCount, int portalCount)
{
	const int cameraBufferElementCount = NTree::CalcTotalElements(portalCount, maxRecursionCount + 1);
	return cameraBufferElementCount;
}

std::optional<PortalManager::RayTraceResult> PortalManager::RayTrace(const Ray& ray, const gsl::span<const TriangleMesh> portalMeshes) const
{
	constexpr int invalidPortalId = -1;
	int bestPortalId = invalidPortalId;
	PortalEndpointIndex bestPortalEndpoint(PortalEndpoint::A);
	float bestPortalDistance = std::numeric_limits<float>::max();
	glm::vec3 bestModelHitLocation;


	for (int portalId = 0; portalId < m_portals.size(); ++portalId)
	{
		const Portal& portal = m_portals[portalId];
		const TriangleMesh& mesh = portalMeshes[portal.meshIndex];

		for (auto endPoint = PortalEndpointIndex::First(); endPoint <= PortalEndpointIndex::Last(); ++endPoint)
		{
			const glm::mat4 inverseModel = glm::inverse(portal.transform[endPoint]);
			const glm::vec3 rayBegin_modelspace = inverseModel * glm::vec4(ray.origin, 1.f);
			const glm::vec3 rayEnd_modelspace = inverseModel * glm::vec4(ray.CalcEndPoint(), 1.f);
			const Ray modelRay = Ray::FromStartAndEndpoint(rayBegin_modelspace, rayEnd_modelspace);
			const std::optional<float> rt_result = mesh.RayTrace(modelRay);
			if (rt_result.has_value() && *rt_result < bestPortalDistance)
			{
				bestPortalDistance = *rt_result;
				bestPortalId = portalId;
				bestPortalEndpoint = endPoint;
				bestModelHitLocation = modelRay.CalcPosition(*rt_result);
			}
		}

	}

	if (bestPortalId != invalidPortalId)
	{
		const glm::vec3 worldspaceHitlocation = glm::vec3(m_portals[bestPortalId].transform[bestPortalEndpoint] * glm::vec4(bestModelHitLocation, 1.f));
		return RayTraceResult{
			glm::distance(ray.origin, worldspaceHitlocation),
			bestPortalId,
			bestPortalEndpoint,
			worldspaceHitlocation
		};
	}

	return std::nullopt;

}

std::optional<glm::mat4> PortalManager::FindHitPortalTeleportMatrix(const Ray& ray, const gsl::span<const TriangleMesh> portalMeshes) const
{
	std::optional<RayTraceResult> rt = RayTrace(ray, portalMeshes);

	if (rt.has_value())
	{
		return m_portals[rt->portalIndex].toOtherEndpoint[rt->endpoint];
	}

	return std::nullopt;
}


