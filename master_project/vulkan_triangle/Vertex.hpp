#pragma once
#include <vulkan/vulkan.hpp>
#include "glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/hash.hpp"
#include "common/VulkanUtils.hpp"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	// maybe move this out, 
	static vk::VertexInputBindingDescription GetBindingDescription()
	{
		vk::VertexInputBindingDescription bindingDescription = vk::VertexInputBindingDescription{}
			.setBinding(0) // all data is interleave in one array
			.setStride(sizeof(Vertex))
			.setInputRate(vk::VertexInputRate::eVertex);

		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions() {
		std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0; // needs to match shader
		attributeDescriptions[0].format = VulkanUtils::GlmTypeToVkFormat<decltype(Vertex::pos)>();
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1; // needs to match shader
		attributeDescriptions[1].format = VulkanUtils::GlmTypeToVkFormat<decltype(Vertex::color)>();
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2; // needs to match shader
		attributeDescriptions[2].format = VulkanUtils::GlmTypeToVkFormat<decltype(Vertex::texCoord)>();
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
		return attributeDescriptions;
	
	}
	
	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
	bool operator!=(const Vertex& other) const { return !(*this  == other); }
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
