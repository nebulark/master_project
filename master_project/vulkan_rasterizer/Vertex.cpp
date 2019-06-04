#include "pch.hpp"
#include "Vertex.hpp"
#include "common/VulkanUtils.hpp"

#pragma warning(push)
#pragma warning(disable: 4573) //Bugged Warning: the usage of 'Vertex::pos' requires the compiler to capture 'this'
const std::array<vk::VertexInputAttributeDescription, 3> Vertex::AttributeDescription = []()
{
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
}();

#pragma warning(pop)


const vk::VertexInputBindingDescription Vertex::vertexBindingDescription_simple = vk::VertexInputBindingDescription{}
			.setBinding(0) // all data is interleave in one array
			.setStride(sizeof(Vertex))
			.setInputRate(vk::VertexInputRate::eVertex);

const vk::PipelineVertexInputStateCreateInfo Vertex::pipelineVertexState_simple = vk::PipelineVertexInputStateCreateInfo{}
		.setVertexBindingDescriptionCount(1)
		.setPVertexBindingDescriptions(&Vertex::vertexBindingDescription_simple)
		.setVertexAttributeDescriptionCount(gsl::narrow<uint32_t>(std::size(Vertex::AttributeDescription)))
		.setPVertexAttributeDescriptions(std::data(Vertex::AttributeDescription))
		;


std::pair<std::vector<Vertex>, std::vector<uint32_t>> Vertex::LoadObjWithIndices(const char* fileName)
{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName);
		if (!success)
		{
			throw std::runtime_error(warn + err);
		}

		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;

		std::unordered_map<Vertex, uint32_t> uniqueVertices;
		for (const tinyobj::shape_t& shape : shapes)
		{
			indices.reserve(shape.mesh.indices.size());

			// we can reuse a vertex approximately 6 times
			vertices.reserve(shape.mesh.indices.size() / 6);

			for (const tinyobj::index_t& index : shape.mesh.indices)
			{
				Vertex vertex = {};
				vertex.pos = {
					attrib.vertices[static_cast<size_t>(3) * index.vertex_index + 0],
					attrib.vertices[static_cast<size_t>(3) * index.vertex_index + 1],
					attrib.vertices[static_cast<size_t>(3) * index.vertex_index + 2],
				};
				// flip y coordinate
				vertex.texCoord = {
					attrib.texcoords[static_cast<size_t>(2) * index.texcoord_index + 0],
					1.f - attrib.texcoords[static_cast<size_t>(2) * index.texcoord_index + 1],
				};

				const uint32_t uniqueIndex = gsl::narrow<uint32_t>(vertices.size());
				const auto [iter, isUnique] = uniqueVertices.try_emplace(vertex, uniqueIndex);
				if (isUnique)
				{
					indices.push_back(uniqueIndex);
					vertices.push_back(vertex);
				}
				else
				{
					assert(uniqueIndex != iter->second);
					indices.push_back(iter->second);
				}
			}
		}
		return std::pair{ std::move(vertices), std::move(indices) };
}
