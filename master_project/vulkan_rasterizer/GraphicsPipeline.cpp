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


std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> GraphicsPipeline::CreateGraphicPipelines_dynamicState(
	const PipelinesCreateInfo& createInfo, uint32_t iterationCount, std::vector<std::string>* optionalDebug /*= nullptr*/)
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

	const vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo_onlyDepthTest = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
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


	const vk::StencilOpState stencilOpState_writeReference = vk::StencilOpState{}
		.setCompareOp(vk::CompareOp::eAlways)
		.setPassOp(vk::StencilOp::eReplace)
		.setFailOp(vk::StencilOp::eReplace)
		.setDepthFailOp(vk::StencilOp::eKeep)
		.setCompareMask(0b1111'1111)
		.setWriteMask(0b1111'1111)
		;

	vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo_portalInitial = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setStencilTestEnable(true)
		.setFront(stencilOpState_writeReference)
		.setBack(stencilOpState_writeReference)
		;


	const vk::StencilOpState stencilOpState_renderifEqual_prototype = vk::StencilOpState{}
		.setCompareOp(vk::CompareOp::eEqual)
		.setPassOp(vk::StencilOp::eKeep)
		.setFailOp(vk::StencilOp::eKeep)
		.setDepthFailOp(vk::StencilOp::eKeep)
		.setWriteMask(0b1111'1111)
		.setCompareMask(0b1111'1111) // will be changed by dynamic state
		.setReference(1)  // will be changed by dynamic state
		;


	const vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo_sceneSubsequent_dynamic_stencilRef_compareMask = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setStencilTestEnable(true)
		.setFront(stencilOpState_renderifEqual_prototype)
		;

	const vk::StencilOpState stencilOpState_writeReferenceIfEqual_prototype = vk::StencilOpState{}
		.setCompareOp(vk::CompareOp::eEqual)
		.setPassOp(vk::StencilOp::eReplace)
		.setFailOp(vk::StencilOp::eKeep)
		.setDepthFailOp(vk::StencilOp::eKeep)
		.setWriteMask(0b1111'1111)
		.setCompareMask(0b1111'1111) // will be changed by dynamic state
		.setReference(1)  // will be changed by dynamic state
		;

	const vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo_portalSubsequent_dynamic_stencilRef_compareMask = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setStencilTestEnable(true)
		.setFront(stencilOpState_writeReferenceIfEqual_prototype)
		.setBack(stencilOpState_writeReferenceIfEqual_prototype)
		;

	const vk::DynamicState dynamicStates[] = {
		vk::DynamicState::eStencilReference,
		vk::DynamicState::eStencilCompareMask, // not really needed, but its convenient
	};

	const vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{}
		.setDynamicStateCount(GetSizeUint32(dynamicStates))
		.setPDynamicStates(dynamicStates)
		;

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_prototype(
		vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
		0, nullptr, // shaderstages, needs to be set!
		&Vertex::pipelineVertexState_simple,
		&inputAssembly_triangleList,
		nullptr, // tesselationstate
		&viewportStateCreateInfo,
		nullptr, // rasterization state create info nees to be set!
		&multisampleState_noMultisampling,
		nullptr, // depthStencilState info nees to be set!
		nullptr, // color blend state needs to be set!
		nullptr, // dynamic device state
		createInfo.pipelineLayout,
		createInfo.renderpass,
		-1, // subpass needs to be set!
		vk::Pipeline(),
		-1);


	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_sceneInitial = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_prototype }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_sceneInitial))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_sceneInitial))
		.setPRasterizationState(&rasterizationStateCreateInfo_scene)
		.setPDepthStencilState(&depthStencilStateCreateInfo_onlyDepthTest)
		.setPColorBlendState(&colorblendstate_override_1)
		.setSubpass(0)
		;



	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_linesInitial = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_sceneInitial }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_linesInitial))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_linesInitial))
		.setPInputAssemblyState(&inputAssembly_lineList)
		.setLayout(createInfo.pipelineLayout_lines)
		.setPVertexInputState(&pipelineVertexState_lines)
		.setSubpass(1)
		;


	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_portalInitial = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_prototype }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_portalInitial))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_portalInitial))
		.setPRasterizationState(&rasterizationStateCreateInfo_portal)
		.setPDepthStencilState(&depthStencilStateCreateInfo_portalInitial)
		.setPColorBlendState(&colorblendstate_override_3)
		.setSubpass(2)
		;

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_sceneSubsequent_prototype = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_prototype }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_sceneSubsequent))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_sceneSubsequent))
		.setPRasterizationState(&rasterizationStateCreateInfo_scene)
		.setPColorBlendState(&colorblendstate_override_1)
		.setPDynamicState(&dynamicStateCreateInfo)
		;

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_portalSubsequent_prototype = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_prototype }
		.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_portalSubsequent))
		.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_portalSubsequent))
		.setPRasterizationState(&rasterizationStateCreateInfo_portal)
		.setPColorBlendState(&colorblendstate_override_3)
		.setPDynamicState(&dynamicStateCreateInfo) 
		// does not really need dynamic state, as the reference value is changed in shader, but we have it for comparemask convenience
		// and the possibility to get it to work without stencil export
		;

	const int shaderCount = 3; // scene, lines, portals


	const uint32_t pipelineCount = ((iterationCount + 1) * shaderCount);
	std::unique_ptr<vk::GraphicsPipelineCreateInfo[]> pipelineCreateInfoStorage = std::make_unique<vk::GraphicsPipelineCreateInfo[]>(pipelineCount);
	gsl::span<vk::GraphicsPipelineCreateInfo> pipelineCreateInfo = gsl::make_span(pipelineCreateInfoStorage.get(), pipelineCount);

	const int firstScenePass = 0;
	const int firstLinePass = firstScenePass + 1;
	const int firstPortalPass = firstLinePass+ 1;

	pipelineCreateInfo[firstScenePass] = graphicsPipelineCreateInfo_sceneInitial;
	pipelineCreateInfo[firstLinePass] = graphicsPipelineCreateInfo_linesInitial;
	pipelineCreateInfo[firstPortalPass] = graphicsPipelineCreateInfo_portalInitial;

	if (optionalDebug)
	{
		optionalDebug->resize(pipelineCount);
		(*optionalDebug)[firstScenePass] = "sceneInitial";
		(*optionalDebug)[firstLinePass] = "linesInitial";
		(*optionalDebug)[firstPortalPass] = "portalInitial";
	}

	for (uint32_t layer = 1; layer <= iterationCount ; ++layer)
	{
		const int renderSceneIdx = layer * shaderCount;
		const int renderLinesIdx = renderSceneIdx + 1;
		const int renderPortalIdx = renderLinesIdx + 1;

		pipelineCreateInfo[renderSceneIdx] = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_sceneSubsequent_prototype }
			.setPDepthStencilState(&depthStencilStateCreateInfo_sceneSubsequent_dynamic_stencilRef_compareMask)
			.setSubpass(renderSceneIdx);

		pipelineCreateInfo[renderLinesIdx] = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_sceneSubsequent_prototype }
			.setPDepthStencilState(&depthStencilStateCreateInfo_sceneSubsequent_dynamic_stencilRef_compareMask)
			.setPInputAssemblyState(&inputAssembly_lineList)
			.setStageCount(GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_linesSubsequent))
			.setPStages(std::data(createInfo.pipelineShaderStageCreationInfos_linesSubsequent))
			.setLayout(createInfo.pipelineLayout_lines)
			.setPVertexInputState(&pipelineVertexState_lines)
			.setSubpass(renderLinesIdx);

		pipelineCreateInfo[renderPortalIdx] = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_portalSubsequent_prototype }
			.setPDepthStencilState(&depthStencilStateCreateInfo_portalSubsequent_dynamic_stencilRef_compareMask)
			.setSubpass(renderPortalIdx);

		if (optionalDebug)
		{
			char buff[128] = {};
			int bytesWritten = 128;

			bytesWritten = std::sprintf(buff, "scene l%i dynamic", layer);
			assert(bytesWritten < std::size(buff));
			(*optionalDebug)[renderSceneIdx] = buff;

			bytesWritten = std::sprintf(buff, "lines l%i dynamic", layer);
			assert(bytesWritten < std::size(buff));
			(*optionalDebug)[renderLinesIdx] = buff;

			bytesWritten = std::sprintf(buff, "portal l%i dynamic", layer);
			assert(bytesWritten < std::size(buff));
			(*optionalDebug)[renderPortalIdx] = buff;
		}
	}

	return createInfo.logicalDevice.createGraphicsPipelinesUnique(
		vk::PipelineCache{},
		vk::ArrayProxy<const vk::GraphicsPipelineCreateInfo>(GetSizeUint32(pipelineCreateInfo), std::data(pipelineCreateInfo)));
}


