#pragma once
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>

namespace GraphicsPipeline
{	
		extern const vk::PipelineColorBlendStateCreateInfo colorblendstate_override_1;
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

		vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> CreateGraphicsPipeline_drawScene_subsequent(
			vk::Device logicalDevice,
			vk::Extent2D swapchainExtent,
			vk::RenderPass renderpass,
			vk::PipelineLayout pipelineLayout,
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos,
			int subpassIndex
		);


		struct PipelinesCreateInfo
		{
			vk::Device logicalDevice;
			vk::Extent2D swapchainExtent;
			vk::RenderPass renderpass;
			vk::PipelineLayout pipelineLayout;

			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_sceneInitial;
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_portalInitial;
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_sceneSubsequent;
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_portalSubsequent;

		};

		std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> CreateGraphicPipelines(const PipelinesCreateInfo& createInfo, uint32_t iterationCount, uint32_t maxPortals);

}
