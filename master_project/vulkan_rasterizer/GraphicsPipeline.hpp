#pragma once
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>

namespace GraphicsPipeline
{	
		extern const vk::PipelineColorBlendStateCreateInfo colorblendstate_override_1;
		extern const vk::PipelineMultisampleStateCreateInfo multisampleState_noMultisampling;
		extern const vk::PipelineInputAssemblyStateCreateInfo inputAssembly_triangleList;

	
		struct ScenePassPipelines
		{
			std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> scene;
			std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> lines;
		};

		struct PortalPassPipelines
		{
			std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> twoSided;
			std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> onlyFront;
			std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> onlyBack;
		};


		struct PipelinesCreateResult
		{
			ScenePassPipelines scenePassPipelines;
			PortalPassPipelines portalPassPipelines;
		};

		struct PipelinesCreateInfo
		{
			vk::Device logicalDevice;
			vk::Extent2D swapchainExtent;
			vk::RenderPass renderpass;
			vk::PipelineLayout pipelineLayout_portal;
			vk::PipelineLayout pipelineLayout_lines;
			vk::PipelineLayout pipelineLayout_scene;

			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_sceneInitial;
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_linesInitial;
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_portalInitial;
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_sceneSubsequent;
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_linesSubsequent;
			gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos_portalSubsequent;

		};

		PipelinesCreateResult CreateGraphicPipelines_dynamicState(const PipelinesCreateInfo& createInfo, uint32_t iterationCount);
}
