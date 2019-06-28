#include "pch.hpp"
#include "GraphicsPipeline.hpp"
#include "Vertex.hpp"
#include "GetSizeUint32.hpp"
#include "NTree.hpp"

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



vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> GraphicsPipeline::CreateGraphicsPipeline_drawScene_initial(
	vk::Device logicalDevice,
	vk::Extent2D swapchainExtent,
	vk::RenderPass renderpass,
	vk::PipelineLayout pipelineLayout,
	gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos)
{
	// fill out when we use vertex data


	const vk::Viewport viewport = vk::Viewport()
		.setX(0.f)
		.setY(0.f)
		.setWidth(static_cast<float>(swapchainExtent.width))
		.setHeight(static_cast<float>(swapchainExtent.height))
		.setMinDepth(0.f)
		.setMaxDepth(1.f);

	const vk::Rect2D scissor(vk::Offset2D(0, 0), swapchainExtent);

	const vk::PipelineViewportStateCreateInfo viewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlags(),
		1, &viewport,
		1, &scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo{}
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

	
	vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		;




	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
		gsl::narrow<uint32_t>(std::size(pipelineShaderStageCreationInfos)), std::data(pipelineShaderStageCreationInfos),
		&Vertex::pipelineVertexState_simple,
		&inputAssembly_triangleList,
		nullptr, // tesselationstate
		&viewportStateCreateInfo,
		&rasterizationStateCreateInfo,
		&multisampleState_noMultisampling,
		&depthStencilStateCreateInfo,
		&colorblendstate_override_1,
		nullptr, // dynamic device state
		pipelineLayout,
		renderpass,
		0,
		vk::Pipeline(),
		-1);
	return logicalDevice.createGraphicsPipelineUnique(vk::PipelineCache(), graphicsPipelineCreateInfo);
}

vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> GraphicsPipeline::CreateGraphicsPipeline_PortalRender_Initial(vk::Device logicalDevice, vk::Extent2D swapchainExtent, vk::RenderPass renderpass, vk::PipelineLayout pipelineLayout, gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos)
{
	const vk::Viewport viewport = vk::Viewport()
		.setX(0.f)
		.setY(0.f)
		.setWidth(static_cast<float>(swapchainExtent.width))
		.setHeight(static_cast<float>(swapchainExtent.height))
		.setMinDepth(0.f)
		.setMaxDepth(1.f);

	const vk::Rect2D scissor(vk::Offset2D(0, 0), swapchainExtent);

	const vk::PipelineViewportStateCreateInfo viewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlags(),
		1, &viewport,
		1, &scissor);

	const vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo{}
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

	vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setStencilTestEnable(true)
.setFront(stencilOpState_writeReference)
.setBack(stencilOpState_writeReference)
		;

	constexpr int subPassIdx = 1;

	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
		gsl::narrow<uint32_t>(std::size(pipelineShaderStageCreationInfos)), std::data(pipelineShaderStageCreationInfos),
		&Vertex::pipelineVertexState_simple,
		&inputAssembly_triangleList,
		nullptr, // tesselationstate
		&viewportStateCreateInfo,
		&rasterizationStateCreateInfo,
		&multisampleState_noMultisampling,
		&depthStencilStateCreateInfo,
		&colorblendstate_override_2,
		nullptr, // dynamic device state
		pipelineLayout,
		renderpass,
		subPassIdx,
		vk::Pipeline(),
		-1);
	return logicalDevice.createGraphicsPipelineUnique(vk::PipelineCache(), graphicsPipelineCreateInfo);

}

vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> GraphicsPipeline::CreateGraphicsPipeline_drawScene_subsequent(vk::Device logicalDevice, vk::Extent2D swapchainExtent, vk::RenderPass renderpass, vk::PipelineLayout pipelineLayout, gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos, int subpassIndex)
{
	// fill out when we use vertex data


	const vk::Viewport viewport = vk::Viewport()
		.setX(0.f)
		.setY(0.f)
		.setWidth(static_cast<float>(swapchainExtent.width))
		.setHeight(static_cast<float>(swapchainExtent.height))
		.setMinDepth(0.f)
		.setMaxDepth(1.f);

	const vk::Rect2D scissor(vk::Offset2D(0, 0), swapchainExtent);

	const vk::PipelineViewportStateCreateInfo viewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlags(),
		1, &viewport,
		1, &scissor);





	vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo{}
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

	vk::StencilOpState stencilOpState_renderifEqual = vk::StencilOpState{}
		.setCompareOp(vk::CompareOp::eEqual)
		.setPassOp(vk::StencilOp::eKeep)
		.setFailOp(vk::StencilOp::eKeep)
		.setDepthFailOp(vk::StencilOp::eKeep)
		.setCompareMask(0b1111'1111)
		.setWriteMask(0b1111'1111)
.setReference(1)
		;

	
	vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
.setStencilTestEnable(true)
.setFront(stencilOpState_renderifEqual)
		;




	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
		gsl::narrow<uint32_t>(std::size(pipelineShaderStageCreationInfos)), std::data(pipelineShaderStageCreationInfos),
		&Vertex::pipelineVertexState_simple,
		&inputAssembly_triangleList,
		nullptr, // tesselationstate
		&viewportStateCreateInfo,
		&rasterizationStateCreateInfo,
		&multisampleState_noMultisampling,
		&depthStencilStateCreateInfo,
		&colorblendstate_override_1,
		nullptr, // dynamic device state
		pipelineLayout,
		renderpass,
		subpassIndex,
		vk::Pipeline(),
		-1);
	return logicalDevice.createGraphicsPipelineUnique(vk::PipelineCache(), graphicsPipelineCreateInfo);

}

std::vector<vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic>> GraphicsPipeline::CreateGraphicPipelines(const PipelinesCreateInfo& createInfo, uint32_t iterationCount, uint32_t maxPortals)
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
		.setCompareMask(0b1111'1111)
		.setWriteMask(0b1111'1111)
		.setReference(1) //////////////////////////////////fix this
		;


	const vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo_sceneSubsequent_prototype = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setStencilTestEnable(true)
		.setFront(stencilOpState_renderifEqual_prototype)
		;

	const vk::StencilOpState stencilOpState_writeReferenceIfEqual_prototype = vk::StencilOpState{}
		.setCompareOp(vk::CompareOp::eEqual)
		.setPassOp(vk::StencilOp::eReplace)
		.setFailOp(vk::StencilOp::eReplace)
		.setDepthFailOp(vk::StencilOp::eKeep)
		.setCompareMask(0b1111'1111) // fix this
		.setWriteMask(0b1111'1111)
		.setReference(1) // fix this
		;

	const vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo_portalSubsequent_prototype = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setStencilTestEnable(true)
		.setFront(stencilOpState_writeReferenceIfEqual_prototype)
		.setBack(stencilOpState_writeReferenceIfEqual_prototype)
		;



	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_sceneInitial(
		vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
		GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_sceneInitial), std::data(createInfo.pipelineShaderStageCreationInfos_sceneInitial),
		&Vertex::pipelineVertexState_simple,
		&inputAssembly_triangleList,
		nullptr, // tesselationstate
		&viewportStateCreateInfo,
		&rasterizationStateCreateInfo_scene,
		&multisampleState_noMultisampling,
		&depthStencilStateCreateInfo_onlyDepthTest,
		&colorblendstate_override_1,
		nullptr, // dynamic device state
		createInfo.pipelineLayout,
		createInfo.renderpass,
		0,
		vk::Pipeline(),
		-1);

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_portalInitial(
		vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
		GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_sceneInitial), std::data(createInfo.pipelineShaderStageCreationInfos_sceneInitial),
		&Vertex::pipelineVertexState_simple,
		&inputAssembly_triangleList,
		nullptr, // tesselationstate
		&viewportStateCreateInfo,
		&rasterizationStateCreateInfo_portal,
		&multisampleState_noMultisampling,
		&depthStencilStateCreateInfo_portalInitial,
		&colorblendstate_override_2,
		nullptr, // dynamic device state
		createInfo.pipelineLayout,
		createInfo.renderpass,
		1,
		vk::Pipeline(),
		-1);


	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_sceneSubsequent_prototype(
		vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
		GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_sceneInitial), std::data(createInfo.pipelineShaderStageCreationInfos_sceneInitial),
		&Vertex::pipelineVertexState_simple,
		&inputAssembly_triangleList,
		nullptr, // tesselationstate
		&viewportStateCreateInfo,
		&rasterizationStateCreateInfo_scene,
		&multisampleState_noMultisampling,
		nullptr, // we will fill this later with the help of depthStencilStateCreateInfo_sceneSubsequent_prototype,
		&colorblendstate_override_1,
		nullptr, // dynamic device state
		createInfo.pipelineLayout,
		createInfo.renderpass,
		-1, // will be set later
		vk::Pipeline(),
		-1);

	const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo_portalSubsequent_prototype(
		vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
		GetSizeUint32(createInfo.pipelineShaderStageCreationInfos_sceneInitial), std::data(createInfo.pipelineShaderStageCreationInfos_sceneInitial),
		&Vertex::pipelineVertexState_simple,
		&inputAssembly_triangleList,
		nullptr, // tesselationstate
		&viewportStateCreateInfo,
		&rasterizationStateCreateInfo_portal,
		&multisampleState_noMultisampling,
		nullptr, // we will fill this later with the help of  depthStencilStateCreateInfo_portalSubsequent_prototype,
		&colorblendstate_override_2,
		nullptr, // dynamic device state
		createInfo.pipelineLayout,
		createInfo.renderpass,
		-1, // will be set later
		vk::Pipeline(),
		-1);

	const uint32_t portalCount = NTree::CalcTotalElements(maxPortals, iterationCount);
	const uint32_t pipelineCount = portalCount * 2;
	std::unique_ptr<vk::GraphicsPipelineCreateInfo[]> pipelineCreateInfoStorage = std::make_unique<vk::GraphicsPipelineCreateInfo[]>(pipelineCount);
	gsl::span<vk::GraphicsPipelineCreateInfo> pipelineCreateInfo = gsl::make_span(pipelineCreateInfoStorage.get(), pipelineCount);


	// need to save this in array as the reference must be kept alive
	const uint32_t depthStencilCiSize = pipelineCount;
	std::unique_ptr<vk::PipelineDepthStencilStateCreateInfo[]> depthStencilCreateInfoStorage = std::make_unique<vk::PipelineDepthStencilStateCreateInfo[]>(depthStencilCiSize);
	gsl::span<vk::PipelineDepthStencilStateCreateInfo> depthStencilCreateInfos = gsl::make_span(depthStencilCreateInfoStorage.get(), depthStencilCiSize);
	int writtenDepthStencilCiCount = 0;

	
	pipelineCreateInfo[0] = graphicsPipelineCreateInfo_sceneInitial;
	pipelineCreateInfo[1] = graphicsPipelineCreateInfo_portalInitial;
	int pipelineCreateInfoCount = 2;

	const uint32_t portalIdBits = GetNumBitsToStoreValue(maxPortals);
	assert(portalIdBits * iterationCount < 8);
	
	const uint32_t shiftsToWriteInFront = 8 - portalIdBits;

	for (uint32_t layer = 1; layer <= iterationCount; ++layer)
	{
		// TODO: could be optimized, especially when if we could assume that maxPortals is power of 2
		const uint32_t layerStartIndex = NTree::CalcFirstLayerIndex(maxPortals, layer);
		const uint32_t layerEndIndex = NTree::CalcFirstLayerIndex(maxPortals, layer + 1);

		const uint8_t compareMask = ~(0b1111'1111 >> ((layer - 1) * portalIdBits));

		for (uint32_t portalIdx = layerStartIndex; portalIdx < layerEndIndex; ++portalIdx)
		{
			NTree::ParentIdxAndChildNum parentChild = NTree::GetParentIdxAndChildNum(maxPortals, portalIdx);

			assert(GetNumBitsToStoreValue(parentChild.childNum) <= 8 - shiftsToWriteInFront);

			// write childnum into stencil value
			uint8_t stencilValue = parentChild.childNum << shiftsToWriteInFront;

			// recursivly add the childnum of parents in fron
			while (parentChild.parentIdx > 0)
			{
				parentChild = NTree::GetParentIdxAndChildNum(maxPortals, parentChild.parentIdx);
				// shift right so that we have space in front to write parentChildNum
				stencilValue = (stencilValue >> portalIdBits) | parentChild.childNum << shiftsToWriteInFront;
			}


			const vk::StencilOpState stencilOpState_scene = vk::StencilOpState{ stencilOpState_renderifEqual_prototype }
				.setCompareMask(compareMask)
				.setReference(stencilValue) 				;
			;

			const vk::StencilOpState stencilOp_portal = vk::StencilOpState{ stencilOpState_writeReferenceIfEqual_prototype }
				.setCompareMask(compareMask)
				.setReference(stencilValue) // technically not needed, as we can set it in shader, if we optimize portal rendering we can't be sure which value is correct
				;

			vk::PipelineDepthStencilStateCreateInfo& depthStencilCreateInfo_scene = depthStencilCreateInfoStorage[writtenDepthStencilCiCount++];
			vk::PipelineDepthStencilStateCreateInfo& depthStencilCreateInfo_portal = depthStencilCreateInfoStorage[writtenDepthStencilCiCount++];

			depthStencilCreateInfo_scene =	vk::PipelineDepthStencilStateCreateInfo(depthStencilStateCreateInfo_sceneSubsequent_prototype)
				.setFront(stencilOpState_scene)
				;


			depthStencilCreateInfo_portal =	vk::PipelineDepthStencilStateCreateInfo(depthStencilStateCreateInfo_portalSubsequent_prototype)
				.setFront(stencilOp_portal)
				.setBack(stencilOp_portal)
				;

			// these are use as supass index as well as to store the Pipeline craete info
			const int renderSceneIdx = pipelineCreateInfoCount++;
			const int renderPortalIdx = pipelineCreateInfoCount++;

			pipelineCreateInfo[renderSceneIdx] = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_sceneSubsequent_prototype }
				.setPDepthStencilState(&depthStencilCreateInfo_scene)
				.setSubpass(renderSceneIdx);

			pipelineCreateInfo[renderPortalIdx] = vk::GraphicsPipelineCreateInfo{ graphicsPipelineCreateInfo_portalSubsequent_prototype }
				.setPDepthStencilState(&depthStencilCreateInfo_portal)
				.setSubpass(renderPortalIdx);

		}
	}

	return createInfo.logicalDevice.createGraphicsPipelinesUnique(
		vk::PipelineCache{},
		vk::ArrayProxy<const vk::GraphicsPipelineCreateInfo>(GetSizeUint32(pipelineCreateInfo), std::data(pipelineCreateInfo)));
}

