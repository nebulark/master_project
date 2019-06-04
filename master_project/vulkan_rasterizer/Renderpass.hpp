#pragma once
struct Renderpass
{

static vk::UniqueRenderPass Create_withDepth(vk::Device logicalDevice, vk::Format swapchainFormat, vk::Format depthFormat);
};
