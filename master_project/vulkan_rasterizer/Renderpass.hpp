#pragma once
struct Renderpass
{

static vk::UniqueRenderPass Create_withDepth(vk::Device logicalDevice, vk::Format swapchainFormat, vk::Format depthFormat);


// Initial Renderpass for Rendering with Portals
// Clears all Buffers, rendered portals need to write into RenderedDepth for later renderpasses,
// but this renderpass does not need to use it RenderedDepth
static vk::UniqueRenderPass Initial_With_Portals(vk::Device logicalDevice);

// Renders Scene geometry only at portal location using the stencil buffer,
// uses RenderedDepth to discard wrong fragments in front of portals
// writes RenderedDepth for later Renderpasses
static vk::UniqueRenderPass Recursion_With_Portals(vk::Device logicalDevice);

// supports currently one iteration with one portal
static vk::UniqueRenderPass Portals_One_Pass(vk::Device logicalDevice, vk::Format colorFormat, vk::Format depthStencilFormat);

};
