#pragma once

#include <vector>
#include <gsl/gsl>
#include <vulkan/vulkan.hpp>
#include "Portal.hpp"

class MeshDataManager;


struct DrawPortalsInfo{
	vk::CommandBuffer drawBuffer;
	MeshDataManager& meshDataManager;
	vk::PipelineLayout layout;
	int iterationElementIndex;
	int numBitsToShiftStencil;
	int maxVisiblePortalCount;
	uint8_t stencilRef;
};


class PortalManager
{
public:
	void Add(const Portal& portal);

	void DrawPortals(vk::CommandBuffer drawBuffer, MeshDataManager& meshDataManager, vk::PipelineLayout layout, int iterationElementIndex, int numBitsToShiftStencil, int maxVisiblePortalCount, uint8_t stencilRef, int firstCameraIndex);
	gsl::span<const Portal> GetPortals() const { return m_portals; }

	void CreateCameraTransforms(
		Transform cameraTransform,
		int maxRecursionCount,
		gsl::span<Transform> outTransforms) const;

	int GetCameraBufferElementCount(int maxRecursionCount) const;

	static int GetCameraBufferElementCount(int maxRecursionCount, int portalCount);
	static int GetPortalIndexHelperElementCount(int maxRecursionCount, int portalCount);


	int GetPortalIndexHelperElementCount(int maxRecursionCount);
	int GetPortalCount() const { return m_portals.size() * 2; }
private:
	std::vector<Portal> m_portals;
};


