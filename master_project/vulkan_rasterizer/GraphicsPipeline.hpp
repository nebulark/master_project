#pragma once
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>

namespace GraphicsPipeline
{
		vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> CreateGraphicsPipeline_static_simple(
			vk::Device logicalDevice,
			vk::Extent2D swapchainExtent,
			vk::RenderPass renderpass,
			vk::PipelineLayout pipelineLayout,
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos
		);

		extern const vk::PipelineColorBlendStateCreateInfo colorblendstate_override;
		extern const vk::PipelineMultisampleStateCreateInfo multisampleState_noMultisampling;
		extern const vk::PipelineInputAssemblyStateCreateInfo inputAssembly_triangleList;
}
