#pragma once
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>
#include <string>
#include "Vertex.hpp"
#include "common/VulkanUtils.hpp"
#include "UniqueVmaObject.hpp"
#include "MeshDataRef.hpp"
#include "glm.hpp"
#include "Transform.hpp"

class MeshDataManager;
struct SceneObject
{
	Transform transform;
	int meshIdx;
	glm::vec4 debugColor;
};

class Scene
{
public:
	Scene(VmaAllocator allocator);

	void Add(int MeshIdx, const Transform& transform, glm::vec4 debugColor = glm::vec4(0.f));
	void Draw(MeshDataManager& meshdataManager, vk::PipelineLayout pipelineLayout, vk::CommandBuffer drawCommandBuffer, uint32_t layerStartIndex, uint32_t layerEndIndex) const;


private:
	
	std::vector<SceneObject> m_objects;

	UniqueVmaBuffer m_drawIndexedIndirectBuffer;
	int m_drawIndexedIndirectBufferElementCount;
	int m_drawIndexedIndirectBufferMaxElements;

	VmaAllocator m_allocator;
};
