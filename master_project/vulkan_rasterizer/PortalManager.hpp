#pragma once

#include <vector>
#include <gsl/gsl>
#include <vulkan/vulkan.hpp>
#include "Portal.hpp"

class MeshDataManager;
class Ray;
class TriangleMesh;


struct DrawPortalsInfo
{
	// command buffer used for drawing
	vk::CommandBuffer drawBuffer;

	// mesh manager that contains the portal meshes, must not be null
	MeshDataManager* meshDataManager;

	// pipeline layout for portal push constants
	vk::PipelineLayout layout;
	int cameraIndex;

	// number of bits we would need to shift a value, so that it won't overwrite important stencil Ref bits
	int numBitsToShiftStencil;

	// the maximum number of visible portals, may be different for each iteration
	int maxVisiblePortalCount;

	// stencil ref for testing, if we should render the portal
	uint8_t stencilCompareValue;

	// the first index in the camera indices array, that should be written by the portals
	int firstCameraIndicesIndex;
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
	static int GetPortalIndexHelperElementCount(int maxRecursionCount, int portalCount);


	int GetPortalIndexHelperElementCount(int maxRecursionCount);
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


