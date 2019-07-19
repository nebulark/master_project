#include "pch.hpp"
#include "LineDrawer.hpp"
#include "PushConstants.hpp"


void LineDrawer::Draw(vk::PipelineLayout pipelineLayout, vk::CommandBuffer drawCommandBuffer, gsl::span<const Line> lines, uint32_t cameraIndexAndStencilCompare)
{
	for (const Line& line : lines)
	{

		PushConstant_lines pushConstant = {};
		pushConstant.posA = line.pointA;
		pushConstant.posB = line.pointB;

		pushConstant.debugColorA = line.colorA;
		pushConstant.debugColorB = line.colorB;

		pushConstant.cameraIndexAndStencilCompare = cameraIndexAndStencilCompare;
		drawCommandBuffer.pushConstants<PushConstant_lines>(
			pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
		drawCommandBuffer.draw(2, 1, 0,0);
	}

}

