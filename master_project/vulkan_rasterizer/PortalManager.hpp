#pragma once

#include <vector>
#include <gsl/gsl>
#include <vulkan/vulkan.hpp>
#include "Portal.hpp"

class MeshDataManager;

class PortalManager
{
public:
	void Add(const Portal& portal);

	void DrawPortals(vk::CommandBuffer drawBuffer, MeshDataManager& meshDataManager, vk::PipelineLayout layout, int iterationElementIndex, std::vector<uint8_t> stencilRefs);
	gsl::span<const Portal> GetPortals() const { return m_portals; }


private:
	std::vector<Portal> m_portals;
};
