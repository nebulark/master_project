#pragma once
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>
#include <string>
#include "Vertex.hpp"
#include "common/VulkanUtils.hpp"
#include "UniqueVmaObject.hpp"
#include "MeshDataRef.hpp"
#include "glm.hpp"

class MeshDataManager;
struct SceneObject
{
	glm::mat4 modelMat;
	int meshIdx;
};

class Scene
{
public:
	Scene(VmaAllocator allocator);

	void Add(int MeshIdx, const glm::mat4& modelmat);
	void Draw(MeshDataManager& meshdataManager, vk::PipelineLayout pipelineLayout, vk::CommandBuffer drawCommandBuffer, uint32_t cameraMatIdx, uint8_t stencil) const;


private:
	
	std::vector<SceneObject> m_objects;

	UniqueVmaBuffer m_drawIndexedIndirectBuffer;
	int m_drawIndexedIndirectBufferElementCount;
	int m_drawIndexedIndirectBufferMaxElements;

	VmaAllocator m_allocator;
};
