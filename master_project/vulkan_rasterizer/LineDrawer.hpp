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

	Line() = default;

	constexpr Line(
		const glm::vec4& pointA,
		const glm::vec4& pointB,
		const glm::vec4& colorA = glm::vec4(1.f,0.f,0.f,1.f),
		const glm::vec4& colorB = glm::vec4(0.f,0.f,1.f,1.f))
		: pointA(pointA)
		, pointB(pointB)
		, colorA(colorA)
		, colorB(colorB)
	{}
};

class LineDrawer
{
public:


	static void Draw(vk::PipelineLayout pipelineLayout, vk::CommandBuffer drawCommandBuffer, uint32_t cameraMatIdx, gsl::span<const Line> lines, uint32_t stencilCompareVal);
};

