#pragma once
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>
#include <string>
#include "Vertex.hpp"
#include "common/VulkanUtils.hpp"
#include "VmaAllocationsPool.hpp"

struct StaticSceneMesh
{
	std::string meshName;
	uint32_t firstIndex;
	uint32_t indexCount;
};

// contains data to draw meshes
// only handles data storage, instancing must be handled somewhere else
class StaticSceneData 
{
public:
	using VertexType = Vertex;
	using IndexType = uint32_t;
	static constexpr vk::IndexType IndexBufferIndexType = VulkanUtils::GetIndexBufferType_v<IndexType>;

	StaticSceneData(VmaBufferPool& allocationPool);

	void LoadObjs(gsl::span<const char* const> objFileNames, vk::Device device,
		vk::CommandPool transferPool, vk::Queue transferQueue);

	StaticSceneData(const StaticSceneData&) = delete;
	StaticSceneData& operator=(const StaticSceneData&) = delete;

	// For now just delete these, we can implement them later
	StaticSceneData(StaticSceneData&& rhs) noexcept = delete;
	StaticSceneData& operator=(StaticSceneData&& rhs) noexcept = delete;

	~StaticSceneData();

	vk::Buffer GetVertexBuffer() { return m_vertexBuffer; }
	vk::Buffer GetIndexBuffer() { return m_indexBuffer; }
	gsl::span<const StaticSceneMesh> GetMeshes() const { return m_meshes; }
private:
	// TODO: Handle Queue Ownerships?


	// stores all vertices of the Scene
	vk::Buffer m_vertexBuffer;

	int m_vertexBufferMaxElements;
	// number of used vertices in Vertex buffer, each Index in IndexBuffer MUST be lower than this value!
	int m_vertexBufferElementCount;

	// stores all indices to the scene,
	// indices are relative to m_vertexBuffer! VertexBufferOffset when drawing should always be 0!
	vk::Buffer m_indexBuffer;
	int m_indexBufferMaxElements;
	// number of used indices in index Buffer
	// when drawing the firstIndex + indexCount MUST be lower or equal than this value!
	int m_indexBufferElementCount;

	std::vector<StaticSceneMesh> m_meshes;

	VmaBufferPool* m_bufferPoolRef;

};


struct bla
{
vk::Buffer m_drawIndexedIndirectBuffer;
	vk::DeviceSize m_drawIndexedIndirectSizeBytes;

	int m_drawIndexedIndirectBufferElementCount;

};
