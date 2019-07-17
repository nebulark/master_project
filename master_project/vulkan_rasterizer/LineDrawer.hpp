#pragma once
#include "glm.hpp"
#include <vector>
#include <vulkan/vulkan.hpp>


struct Line
{
	glm::vec4 pointA;
	glm::vec4 pointB;
	glm::vec4 colorA;
	glm::vec4 colorB;
};

class LineDrawer
{
public:


	static void Draw(vk::PipelineLayout pipelineLayout, vk::CommandBuffer drawCommandBuffer, uint32_t cameraMatIdx, gsl::span<const Line> lines);
};

