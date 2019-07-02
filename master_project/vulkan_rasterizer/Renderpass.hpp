#pragma once
struct Renderpass
{
	static vk::UniqueRenderPass Portals_One_Pass_dynamicState(vk::Device logicalDevice, vk::Format colorFormat, vk::Format depthStencilFormat, int iterationCount, std::vector<std::string>* optionalDebug = nullptr);
};
