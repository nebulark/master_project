#pragma once
#include <vulkan/vulkan.hpp>
#include "glm.hpp"
#include "gtx/hash.hpp"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static const std::array<vk::VertexInputAttributeDescription, 3> AttributeDescription;

	// assumes all data is interleaved in a single array and there is no special instancing stuff
	static const vk::VertexInputBindingDescription vertexBindingDescription_simple;
	static const vk::PipelineVertexInputStateCreateInfo pipelineVertexState_simple;

	static std::pair<std::vector<Vertex>, std::vector<uint32_t>> LoadObjWithIndices(const char* fileName, int firstIndex = 0);

	// appends vertices and indices to the given arrays
	// indices will corespont to the index in vertices + firstIndex
	// if you want to pass append to vertices and start indices with zero, you should pass -vertices.size() as the first index parameter!
	static void LoadObjWithIndices_append(const char* fileName, std::vector<Vertex>& inOutVertices, std::vector<uint32_t>& inOutIndices, int firstIndex = 0);

	bool operator==(const Vertex& other) const {
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
	}
	bool operator!=(const Vertex& other) const { return !(*this == other); }
};


namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>{}.operator()(vertex.pos) ^
				(hash<glm::vec3>{}.operator()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>{}.operator()(vertex.texCoord) << 1);
		}
	};
}
