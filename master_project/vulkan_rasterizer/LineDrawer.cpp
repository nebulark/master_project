#include "pch.hpp"
#include "LineDrawer.hpp"
#include "PushConstants.hpp"

void LineDrawer::Draw(vk::PipelineLayout pipelineLayout, vk::CommandBuffer drawCommandBuffer, uint32_t cameraMatIdx) const
{
	for (const Line& line : m_lines)
	{

		PushConstant_lines pushConstant = {};
		pushConstant.cameraIdx  = cameraMatIdx;
		pushConstant.posA = line.pointA;
		pushConstant.posB = line.pointB;

		pushConstant.debugColorA = line.colorA;
		pushConstant.debugColorB = line.colorB;

		drawCommandBuffer.pushConstants<PushConstant_lines>(
			pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
		drawCommandBuffer.draw(2, 1, 0,0);
	}

}
