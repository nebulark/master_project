#pragma once
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>

namespace GraphicsPipeline
{	
		extern const vk::PipelineColorBlendStateCreateInfo colorblendstate_override;
		extern const vk::PipelineMultisampleStateCreateInfo multisampleState_noMultisampling;
		extern const vk::PipelineInputAssemblyStateCreateInfo inputAssembly_triangleList;

		vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> CreateGraphicsPipeline_drawScene_initial(
			vk::Device logicalDevice,
			vk::Extent2D swapchainExtent,
			vk::RenderPass renderpass,
			vk::PipelineLayout pipelineLayout,
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos
		);

			
		vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> CreateGraphicsPipeline_PortalRender_Initial(
			vk::Device logicalDevice,
			vk::Extent2D swapchainExtent,
			vk::RenderPass renderpass,
			vk::PipelineLayout pipelineLayout,
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos
		);
}
