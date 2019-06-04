#pragma once
#include <vulkan/vulkan.hpp>
#include "glm.hpp"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static const std::array<vk::VertexInputAttributeDescription, 3> AttributeDescription;

	// assumes all data is interleaved in a single array and there is no special instancing stuff
	static const vk::VertexInputBindingDescription vertexBindingDescription_simple;
	static const vk::PipelineVertexInputStateCreateInfo pipelineVertexState_simple;

	static std::pair<std::vector<Vertex>, std::vector<uint32_t>> LoadObjWithIndices(const char* fileName);

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
	bool operator!=(const Vertex& other) const { return !(*this == other); }
};


namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}
