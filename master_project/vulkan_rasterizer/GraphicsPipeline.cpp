#include "pch.hpp"
#include "GraphicsPipeline.hpp"
#include "Vertex.hpp"
#include "GetSizeUint32.hpp"
#include "NTree.hpp"
#include <charconv>

namespace
{

	std::array<vk::PipelineColorBlendAttachmentState, 4>  colorblend_override_arr4 = []()
	{
	vk::PipelineColorBlendAttachmentState colorblend_override = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(vk::ColorComponentFlags()
			| vk::ColorComponentFlagBits::eR
			| vk::ColorComponentFlagBits::eG
			| vk::ColorComponentFlagBits::eB
			| vk::ColorComponentFlagBits::eA)
		.setBlendEnable(false)
		.setSrcColorBlendFactor(vk::BlendFactor::eOne)
		.setDstColorBlendFactor(vk::BlendFactor::eZero)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd);


	std::array<vk::PipelineColorBlendAttachmentState, 4>  colorblend_override_arr4;
	colorblend_override_arr4.fill(colorblend_override);
	return colorblend_override_arr4;

	}();
	
	const vk::PipelineInputAssemblyStateCreateInfo inputAssembly_lineList = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eLineList,
		false
	);

	const vk::PipelineVertexInputStateCreateInfo pipelineVertexState_lines = vk::PipelineVertexInputStateCreateInfo{}
		.setVertexBindingDescriptionCount(0)
		.setPVertexBindingDescriptions(nullptr)
		.setVertexAttributeDescriptionCount(0)
		.setPVertexAttributeDescriptions(nullptr)
		;


}

const vk::PipelineColorBlendStateCreateInfo GraphicsPipeline::colorblendstate_override_1 = vk::PipelineColorBlendStateCreateInfo()
	.setLogicOpEnable(false)
		.setLogicOp(vk::LogicOp::eCopy)
		.setAttachmentCount(1)
		.setPAttachments(colorblend_override_arr4.data())
		.setBlendConstants({ 0.f,0.f,0.f,0.f })
		;

const vk::PipelineColorBlendStateCreateInfo colorblendstate_override_2 = vk::PipelineColorBlendStateCreateInfo()
	.setLogicOpEnable(false)
		.setLogicOp(vk::LogicOp::eCopy)
		.setAttachmentCount(2)
		.setPAttachments(colorblend_override_arr4.data())
		.setBlendConstants({ 0.f,0.f,0.f,0.f })
		;
const vk::PipelineColorBlendStateCreateInfo colorblendstate_override_3 = vk::PipelineColorBlendStateCreateInfo()
	.setLogicOpEnable(false)
		.setLogicOp(vk::LogicOp::eCopy)
		.setAttachmentCount(3)
		.setPAttachments(colorblend_override_arr4.data())
		.setBlendConstants({ 0.f,0.f,0.f,0.f })
		;


const vk::PipelineMultisampleStateCreateInfo GraphicsPipeline::multisampleState_noMultisampling = vk::PipelineMultisampleStateCreateInfo()
		.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setMinSampleShading(1.f)
		.setPSampleMask(nullptr)
		.setAlphaToCoverageEnable(false)
		.setAlphaToOneEnable(false);

const vk::PipelineInputAssemblyStateCreateInfo GraphicsPipeline::inputAssembly_triangleList = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eTriangleList,
		false
	);


GraphicsPipeline::PipelinesCreateResult GraphicsPipeline::CreateGraphicPipelines_dynamicState(
	const PipelinesCreateInfo& createInfo, uint32_t iterationCount)
{
	const vk::Viewport viewport = vk::Viewport()
		.setX(0.f)
		.setY(0.f)
		.setWidth(static_cast<float>(createInfo.swapchainExtent.width))
		.setHeight(static_cast<float>(createInfo.swapchainExtent.height))
		.setMinDepth(0.f)
		.setMaxDepth(1.f);

	const vk::Rect2D scissor(vk::Offset2D(0, 0), createInfo.swapchainExtent);

	const vk::PipelineViewportStateCreateInfo viewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlags(),
		1, &viewport,
		1, &scissor);


	const vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo_scene = vk::PipelineRasterizationStateCreateInfo{}
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1.f)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.f)
		.setDepthBiasClamp(0.f)
		.setDepthBiasSlopeFactor(0.f);

	const vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo_onlyDepthTest = 
		vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setStencilTestEnable(false)
		;
	const vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo_portal = vk::PipelineRasterizationStateCreateInfo{}
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1.f)
		.setCullMode(vk::CullModeFlagBits::eNone)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.f)
		.setDepthBiasClamp(0.f)
		.setDepthBiasSlopeFactor(0.f);

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_prototype(
		vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
		0, nullptr, // shaderstages, needs to be set!
		&Vertex::pipelineVertexState_simple,
		&inputAssembly_triangleList,
		nullptr, // tesselationstate
		&viewportStateCreateInfo,
		nullptr, // rasterization state create info needs to be set!
		&multisampleState_noMultisampling,
		nullptr, // depthStencilState info needs to be set!
		nullptr, // color blend state needs to be set!
		nullptr, // dynamic device state
		nullptr,
		createInfo.renderpass,
		-1, // subpass needs to be set!
		vk::Pipeline(),
		-1);


	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_sceneInitial = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_prototype }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_sceneInitial))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_sceneInitial))
		.setPRasterizationState(&rasterizationStateCreateInfo_scene)
		.setPDepthStencilState(&depthStencilStateCreateInfo_onlyDepthTest)
		.setLayout(createInfo.pipelineLayout_scene)
		.setPColorBlendState(&colorblendstate_override_1)
		.setSubpass(0)
		;

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_sceneSubsequent_prototype = 
		vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_sceneInitial }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_sceneSubsequent))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_sceneSubsequent))
		.setSubpass(-1)
		;

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_linesInitial = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_prototype }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_linesInitial))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_linesInitial))
		.setPRasterizationState(&rasterizationStateCreateInfo_scene)
		.setPDepthStencilState(&depthStencilStateCreateInfo_onlyDepthTest)
		.setPInputAssemblyState(&inputAssembly_lineList)
		.setLayout(createInfo.pipelineLayout_lines)
		.setPVertexInputState(&pipelineVertexState_lines)
		.setPColorBlendState(&colorblendstate_override_1)
		.setSubpass(0)
		;

		const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_linesSubsequent_prototype = 
			vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_linesInitial }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_linesInitial))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_linesInitial))
		.setSubpass(-1)
		;

/////

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_portalInitial = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_prototype }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_portalInitial))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_portalInitial))
		.setPRasterizationState(&rasterizationStateCreateInfo_portal)
		.setPDepthStencilState(&depthStencilStateCreateInfo_onlyDepthTest)
		.setLayout(createInfo.pipelineLayout_portal)
		.setPColorBlendState(&colorblendstate_override_3)
		.setSubpass(1)
		;

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_portalSubsequent_prototype =
		vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_portalInitial }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_portalSubsequent))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_portalSubsequent))
		.setSubpass(-1)
	;



	vk::PipelineCache cache;

	vk::GraphicsPipelineCreateInfo intitialCreateInfos[] =
	{
		graphicsPipelineCreateInfo_sceneInitial,
		graphicsPipelineCreateInfo_linesInitial,
		graphicsPipelineCreateInfo_portalInitial,
	};


	std::vector<vk::GraphicsPipelineCreateInfo> linePipelineCreateInfos;

	auto intialPipelines =
		createInfo.logicalDevice.createGraphicsPipelinesUnique(cache, { std::size(intitialCreateInfos), std::data(intitialCreateInfos) });

	PipelinesCreateResult result;
	result.scenePassPipelines.scene.push_back(std::move(intialPipelines[0]));
	result.scenePassPipelines.lines.push_back(std::move(intialPipelines[1]));
	result.portalPassPipelines.regularPortal.push_back(std::move(intialPipelines[2]));

	std::vector<vk::GraphicsPipelineCreateInfo> scenePipelineCreateInfos(iterationCount);
	std::vector<vk::GraphicsPipelineCreateInfo> linePipelineCreateInfoss(iterationCount);
	std::vector<vk::GraphicsPipelineCreateInfo> portalPipelineCreateInfos(iterationCount);

	for (uint32_t layer = 1; layer <= iterationCount ; ++layer)
	{
		const int sceneSubpassIndex = layer * 2;
		const int portalSubpassIndex = sceneSubpassIndex + 1;
		const int creationInfoIndex = layer - 1;

		scenePipelineCreateInfos[creationInfoIndex] = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_sceneSubsequent_prototype }
			.setSubpass(sceneSubpassIndex);

		linePipelineCreateInfoss[creationInfoIndex] = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_linesSubsequent_prototype }
			.setSubpass(sceneSubpassIndex);

		portalPipelineCreateInfos[creationInfoIndex] = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_portalSubsequent_prototype }
			.setSubpass(portalSubpassIndex);
	}

	auto scenePipes = createInfo.logicalDevice.createGraphicsPipelinesUnique(cache, scenePipelineCreateInfos);
	auto linePipes = createInfo.logicalDevice.createGraphicsPipelinesUnique(cache, linePipelineCreateInfoss);
	auto portalPipes = createInfo.logicalDevice.createGraphicsPipelinesUnique(cache, portalPipelineCreateInfos);

	const auto append = [](
		std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>>& base,
		std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>>&& toAppend)
	{
		base.insert(base.end(), std::make_move_iterator(toAppend.begin()), std::make_move_iterator(toAppend.end()));
	};

	append(result.scenePassPipelines.scene, std::move(scenePipes));
	append(result.scenePassPipelines.lines, std::move(linePipes));
	append(result.portalPassPipelines.regularPortal, std::move(portalPipes));
	return result;
}


