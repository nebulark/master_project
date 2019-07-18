#pragma once
struct Renderpass
{
	static vk::UniqueRenderPass Portals_One_Pass_dynamicState(vk::Device logicalDevice, vk::Format colorFormat, vk::Format depthStencilFormat, vk::Format renderedDepthFormat, vk::Format renderedStencilFormat, int iterationCount);
};
