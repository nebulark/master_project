#pragma once

#include <vector>
#include <gsl/gsl>
#include <vulkan/vulkan.hpp>
#include "Portal.hpp"

class MeshDataManager;
struct Ray;
class TriangleMesh;


struct DrawPortalsInfo
{
	// command buffer used for drawing
	vk::CommandBuffer drawBuffer;

	// mesh manager that contains the portal meshes, must not be null
	MeshDataManager* meshDataManager;

	// pipeline layout for portal push constants
	vk::PipelineLayout layout;

	// the maximum number of visible portals, may be different for each iteration
	int maxVisiblePortalCount;

	int layerStartIndex;
	int nextLayerStartIndex;
};



class PortalManager
{
public:
	void Add(const Portal& portal);

	void DrawPortals(const DrawPortalsInfo& info);
	gsl::span<const Portal> GetPortals() const { return m_portals; }

	void CreateCameraMats(
		glm::mat4 cameraMat,
		int maxRecursionCount,
		gsl::span<glm::mat4> outCameraTransforms) const;


	int GetCameraBufferElementCount(int maxRecursionCount) const;

	static int GetCameraBufferElementCount(int maxRecursionCount, int portalCount);


	int GetPortalCount() const { return m_portals.size() * 2; }

	struct RayTraceResult
	{
		float t;
		int portalIndex;
		PortalEndpointIndex endpoint;
		glm::vec3 hitLocation;
	};


	std::optional<RayTraceResult> RayTrace(const Ray& ray, const gsl::span<const TriangleMesh> portalMeshes) const;
	std::optional<glm::mat4> FindHitPortalTeleportMatrix(const Ray& ray, const gsl::span<const TriangleMesh> portalMeshes) const;
private:
	std::vector<Portal> m_portals;
};


