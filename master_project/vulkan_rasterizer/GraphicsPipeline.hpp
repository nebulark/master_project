#pragma once
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>

namespace GraphicsPipeline
{	
		extern const vk::PipelineColorBlendStateCreateInfo colorblendstate_override_1;
		extern const vk::PipelineMultisampleStateCreateInfo multisampleState_noMultisampling;
		extern const vk::PipelineInputAssemblyStateCreateInfo inputAssembly_triangleList;

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

		std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> CreateGraphicPipelines_dynamicState(const PipelinesCreateInfo& createInfo, uint32_t iterationCount, std::vector<std::string>* optionalDebug = nullptr);
}
