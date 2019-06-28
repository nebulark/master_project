#include "pch.hpp"
#include "Renderpass.hpp"
#include "GetSizeUint32.hpp"
#include "NTree.hpp"




vk::UniqueRenderPass Renderpass::Create_withDepth(vk::Device logicalDevice, vk::Format colorFormat, vk::Format depthFormat)
{
	vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
		.setFormat(colorFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
		;

	vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
		.setFormat(depthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		;
	// has something to do with frag shader layout 0
	vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::SubpassDescription subpassDescription = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&colorAttachmentRef)
		.setPDepthStencilAttachment(&depthAttachmentRef)
		;

	// ensures we wait on the swapchain image
	vk::SubpassDependency subpassDependency = vk::SubpassDependency{}
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

	std::array<vk::AttachmentDescription, 2> attackments = { colorAttachment, depthAttachment };

	vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(static_cast<uint32_t>(attackments.size())).setPAttachments(attackments.data())
		.setSubpassCount(1).setPSubpasses(&subpassDescription)
		.setDependencyCount(1).setPDependencies(&subpassDependency)
		;

	return logicalDevice.createRenderPassUnique(renderPassCreateInfo);
}


vk::UniqueRenderPass Renderpass::Portals_One_Pass_old(vk::Device logicalDevice, vk::Format colorFormat, vk::Format depthStencilFormat)
{
	// TODO: Make these parameters?
	constexpr int Max_Visible_Portals = 1;
	constexpr int Max_Recursion = 2;

	vk::Format renderedDepthFormat = vk::Format::eR32Sfloat;

	enum AttachmentDescriptionIdx
	{
		color,
		depthStencil,
		renderedDepth_0,
		renderedDepth_1,
		enum_size_AttachmentDescriptionIdx
	};



	static_assert(renderedDepth_0 + 1 == renderedDepth_1);

	const vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;

	std::array<vk::AttachmentDescription, AttachmentDescriptionIdx::enum_size_AttachmentDescriptionIdx> attachmentsDescritpions;

	attachmentsDescritpions[color] = vk::AttachmentDescription()
		.setFormat(colorFormat)
		.setSamples(samples)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
		;

	attachmentsDescritpions[depthStencil] = vk::AttachmentDescription()
		.setFormat(depthStencilFormat)
		.setSamples(samples)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		;

	attachmentsDescritpions[renderedDepth_0] = vk::AttachmentDescription()
		.setFormat(renderedDepthFormat)
		.setSamples(samples)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
		;

	attachmentsDescritpions[renderedDepth_1] = attachmentsDescritpions[renderedDepth_0];

	enum SubpassDescriptionIdx
	{
		scene_initial,
		portal_initial,

		scene_portal0,
		//portal_portal0,
		enum_size_SubpassDescriptionIdx
	};

	std::array<vk::SubpassDescription, SubpassDescriptionIdx::enum_size_SubpassDescriptionIdx> subpassDescriptions;


	// intial scene subpass
	const vk::AttachmentReference initialSceneSubpassReferences_color[] = {
		vk::AttachmentReference(color, vk::ImageLayout::eColorAttachmentOptimal),
	};
	const vk::AttachmentReference initialSceneSubpassReferences_depthStencil(
		depthStencil, vk::ImageLayout::eDepthStencilAttachmentOptimal
	);


	subpassDescriptions[scene_initial] = vk::SubpassDescription{}
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(GetSizeUint32(initialSceneSubpassReferences_color)).setPColorAttachments(initialSceneSubpassReferences_color)
		.setPDepthStencilAttachment(&initialSceneSubpassReferences_depthStencil)
		;

	// first Portal sub pass
	const vk::AttachmentReference firstPortalSubpassReferences_color[] = {
		vk::AttachmentReference(color, vk::ImageLayout::eColorAttachmentOptimal), // TODO: Put this into preserve attachments, for now we keep it for debugging
		vk::AttachmentReference(renderedDepth_0, vk::ImageLayout::eColorAttachmentOptimal),
	};
	const vk::AttachmentReference firstPortalSubpassReferences_depthStencil(
		depthStencil, vk::ImageLayout::eDepthStencilAttachmentOptimal
	);

	subpassDescriptions[portal_initial] = vk::SubpassDescription{}
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(GetSizeUint32(firstPortalSubpassReferences_color)).setPColorAttachments(firstPortalSubpassReferences_color)
		.setPDepthStencilAttachment(&firstPortalSubpassReferences_depthStencil)
		;


	// subsequent scene Subpass
	const vk::AttachmentReference subsequentSceneSubpassReferences_color[] = {
		vk::AttachmentReference(color, vk::ImageLayout::eColorAttachmentOptimal),
	};

	const vk::AttachmentReference subsequentSceneSubpassReferences_input[] = {
		vk::AttachmentReference(renderedDepth_0, vk::ImageLayout::eShaderReadOnlyOptimal),
	};

	const vk::AttachmentReference subsequentSceneSubpassReferences_depthStencil(
		depthStencil, vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal
	);

	subpassDescriptions[scene_portal0]  = vk::SubpassDescription{}
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(GetSizeUint32(subsequentSceneSubpassReferences_color)).setPColorAttachments(subsequentSceneSubpassReferences_color)
		.setInputAttachmentCount(GetSizeUint32(subsequentSceneSubpassReferences_input)).setPInputAttachments(subsequentSceneSubpassReferences_input)
		.setPDepthStencilAttachment(&subsequentSceneSubpassReferences_depthStencil)
		;

	// subsequent portal Subpass

	const vk::AttachmentReference subsequentPortalSubpassReferences_color[] = {
		vk::AttachmentReference(color, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(renderedDepth_1, vk::ImageLayout::eColorAttachmentOptimal)
	};

	const vk::AttachmentReference subsequentPortalSubpassReferences_input[] = {
		vk::AttachmentReference(renderedDepth_0, vk::ImageLayout::eShaderReadOnlyOptimal),
	};

	const vk::AttachmentReference subsequentPortalSubpassReferences_depthStencil(
		depthStencil, vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal
	);

	/*subpassDescriptions[portal_portal0] = vk::SubpassDescription{}
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(GetSizeUint32(subsequentPortalSubpassReferences_color)).setPColorAttachments(subsequentPortalSubpassReferences_color)
		.setInputAttachmentCount(GetSizeUint32(subsequentPortalSubpassReferences_input)).setPInputAttachments(subsequentPortalSubpassReferences_input)
		.setPDepthStencilAttachment(&subsequentPortalSubpassReferences_depthStencil)
		;*/


	std::array<vk::SubpassDependency, 4> dependencies;
	dependencies[0] = vk::SubpassDependency{}
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(scene_initial)
		.setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
	;

	dependencies[1] = vk::SubpassDependency{}
		.setSrcSubpass(scene_initial)
		.setDstSubpass(portal_initial)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
		.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
		.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
	;

	// use color output from previous as input (color write -> shader read)
	dependencies[2] = vk::SubpassDependency{}
		.setSrcSubpass(portal_initial)
		.setDstSubpass(scene_portal0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
		.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
		.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
	;


	dependencies.back() = vk::SubpassDependency{}
	.setSrcSubpass(SubpassDescriptionIdx::enum_size_SubpassDescriptionIdx - 1)
		.setDstSubpass(VK_SUBPASS_EXTERNAL)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
		.setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
	;


	vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(GetSizeUint32(attachmentsDescritpions)).setPAttachments(std::data(attachmentsDescritpions))
		.setSubpassCount(GetSizeUint32(subpassDescriptions)).setPSubpasses(std::data(subpassDescriptions))
		.setDependencyCount(GetSizeUint32(dependencies)).setPDependencies(std::data(dependencies))
		;

	return logicalDevice.createRenderPassUnique(renderPassCreateInfo);

}








vk::UniqueRenderPass Renderpass::Portals_One_Pass(vk::Device logicalDevice, vk::Format colorFormat, vk::Format depthStencilFormat, int maxPortals, int iterationCount)
{

	vk::Format renderedDepthFormat = vk::Format::eR32Sfloat;

	enum AttachmentDescriptionIdx
	{
		color,
		depthStencil,
		renderedDepth_0,
		renderedDepth_1,
		enum_size_AttachmentDescriptionIdx
	};

	static_assert(renderedDepth_0 + 1 == renderedDepth_1);

	constexpr vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;

	const std::array<vk::AttachmentDescription, AttachmentDescriptionIdx::enum_size_AttachmentDescriptionIdx> attachmentsDescritpions = 
	[samples, colorFormat, depthStencilFormat, renderedDepthFormat]()
	{
		std::array<vk::AttachmentDescription, AttachmentDescriptionIdx::enum_size_AttachmentDescriptionIdx> attachmentsDescritpions;

		attachmentsDescritpions[color] = vk::AttachmentDescription()
			.setFormat(colorFormat)
			.setSamples(samples)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
			;

		attachmentsDescritpions[depthStencil] = vk::AttachmentDescription()
			.setFormat(depthStencilFormat)
			.setSamples(samples)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			;

		attachmentsDescritpions[renderedDepth_0] = vk::AttachmentDescription()
			.setFormat(renderedDepthFormat)
			.setSamples(samples)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
			;

		attachmentsDescritpions[renderedDepth_1] = attachmentsDescritpions[renderedDepth_0];

		return attachmentsDescritpions;
	}();


	//////////////////////////////////////////////////////////////////////////
	// Subpass Description

	// *2 because scene + portalSubpass
	const uint32_t subpassCount = NTree::CalcTotalElements(maxPortals, iterationCount) * 2;
	std::unique_ptr<vk::SubpassDescription[]> subpassStorage = std::make_unique<vk::SubpassDescription[]>(subpassCount);
	gsl::span<vk::SubpassDescription> subpasses = gsl::make_span(subpassStorage.get(), subpassCount);




	// for render scene we will  always have this output
	const vk::AttachmentReference renderSceneOutput[] = {
		vk::AttachmentReference(color, vk::ImageLayout::eColorAttachmentOptimal),
	};

	// we will always use the same depth stencil attachment
	const vk::AttachmentReference depthStencilAttachment = vk::AttachmentReference(
		depthStencil, vk::ImageLayout::eDepthStencilAttachmentOptimal
	);

	// used to switch between rendered Depth 0 and rendered Depth 1, we switch after each iteration
	// withing the same iteration no two draws will touch the same pixel, so its save
	enum IterationParity
	{
		even = 0,
		odd,
		enum_size_IterationParity
	};

	const std::array<std::array<vk::AttachmentReference,2>, enum_size_IterationParity>  renderPortalOutput = []()
	{
		std::array<std::array<vk::AttachmentReference,2>, enum_size_IterationParity> renderPortalOutput;
		renderPortalOutput[even] = 
		{
			vk::AttachmentReference(color, vk::ImageLayout::eColorAttachmentOptimal), // TODO: Put this into preserve attachments, for now we keep it for debugging
			vk::AttachmentReference(renderedDepth_0, vk::ImageLayout::eColorAttachmentOptimal),
		};
	
		renderPortalOutput[odd] = 
		{
			vk::AttachmentReference(color, vk::ImageLayout::eColorAttachmentOptimal), // TODO: Put this into preserve attachments, for now we keep it for debugging
			vk::AttachmentReference(renderedDepth_1, vk::ImageLayout::eColorAttachmentOptimal),
		};


		return renderPortalOutput;
	}();

	// input attachments all subsequent passes will use (scene AND portal), take care that renderedDepth is the opposite of renderPortalOutput attachments!
	const std::array<std::array<vk::AttachmentReference,1>, enum_size_IterationParity> subsequentPassInput = []()
	{
		std::array<std::array<vk::AttachmentReference,1>, enum_size_IterationParity> subsequentPassInput;

		subsequentPassInput[even] = {
			vk::AttachmentReference(renderedDepth_1, vk::ImageLayout::eShaderReadOnlyOptimal),
		};

		subsequentPassInput[odd] = {
			vk::AttachmentReference(renderedDepth_0, vk::ImageLayout::eShaderReadOnlyOptimal),
		};

		return subsequentPassInput;
	}();

	// don't know how many there are, maybe test it?
	std::vector<vk::SubpassDependency> dependencies;


	// initial subpasses
	// intial scene subpass
	// iteration 0, this means use even where neccessary

	constexpr int initialSceneSubpassIdx = 0;
	subpasses[initialSceneSubpassIdx] = vk::SubpassDescription{}
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(GetSizeUint32(renderSceneOutput)).setPColorAttachments(renderSceneOutput)
		.setPDepthStencilAttachment(&depthStencilAttachment)
		;

	dependencies.push_back(vk::SubpassDependency{}
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(initialSceneSubpassIdx)
		.setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
	);


	// first Portal sub pass

	constexpr int initialPortalSubpassIdx = initialSceneSubpassIdx + 1;
	subpasses[initialPortalSubpassIdx] = vk::SubpassDescription{}
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(GetSizeUint32(renderPortalOutput[even])).setPColorAttachments(std::data(renderPortalOutput[even]))
		.setPDepthStencilAttachment(&depthStencilAttachment)
		;

	// we need to wait for the depth buffer write (and maybe color) before rendering portal
	dependencies.push_back(vk::SubpassDependency{}
		.setSrcSubpass(initialSceneSubpassIdx)
		.setDstSubpass(initialPortalSubpassIdx)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
		.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
		.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
	);


	int clearAttachmentSubpasses = 0;

	// subsequent subpasses
	for (int iteration = 1; iteration <= iterationCount; ++iteration)
	{

		const IterationParity iterationParity = iteration % 2 == 0 ? even : odd;
		const int firstLayerIdx = NTree::CalcFirstLayerIndex(maxPortals, iteration) * 2;
		const int onePastLastLayerIdx = NTree::CalcFirstLayerIndex(maxPortals, iteration) * 2;

		// this one is responsible for clearing depth buffer
		// it depends on all other portal subpasses in that layer
		// all scene passes of the next layer depend on it
		const int lastPortalSubpassIdx = onePastLastLayerIdx - 1;

		const int previousLastPortalSubpassIdx = firstLayerIdx - 1;
		
		// i+=2 as we are doing scene and portal subpass in one loop
		for (int subpassdx = firstLayerIdx; subpassdx < onePastLastLayerIdx; subpassdx += 2)
		{
			const int sceneSubpassIdx = subpassdx;
			const int portalSubpassIdx = subpassdx + 1;
			assert(portalSubpassIdx >= onePastLastLayerIdx);

			subpasses[sceneSubpassIdx] = vk::SubpassDescription{}
				.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
				.setColorAttachmentCount(GetSizeUint32(renderSceneOutput)).setPColorAttachments(renderSceneOutput)
				.setInputAttachmentCount(GetSizeUint32(subsequentPassInput[iterationParity]))
				.setPInputAttachments(std::data(subsequentPassInput[iterationParity]))
				.setPDepthStencilAttachment(&depthStencilAttachment)
				; 
		
			// we depend on the last portal pass from the previous layer, which is responsible for clearing depth
			dependencies.push_back(vk::SubpassDependency{}
				.setSrcSubpass(previousLastPortalSubpassIdx)
				.setDstSubpass(sceneSubpassIdx)
				.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
				.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
			);

			subpasses[portalSubpassIdx] = vk::SubpassDescription{}
				.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
				.setColorAttachmentCount(GetSizeUint32(renderPortalOutput[iterationParity]))
				.setPColorAttachments(std::data(renderPortalOutput[iterationParity]))
				.setInputAttachmentCount(GetSizeUint32(subsequentPassInput[iterationParity]))
				.setPInputAttachments(std::data(subsequentPassInput[iterationParity]))
				.setPDepthStencilAttachment(&depthStencilAttachment)
				;

			// we need to wait for the depth buffer write (and maybe color) before rendering portal
			dependencies.push_back(vk::SubpassDependency{}
				.setSrcSubpass(sceneSubpassIdx)
				.setDstSubpass(portalSubpassIdx)
				.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
				.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
			);

			// the last portal subpass depends on us
			if (portalSubpassIdx != lastPortalSubpassIdx)
			{
			dependencies.push_back(vk::SubpassDependency{}
				.setSrcSubpass(portalSubpassIdx)
				.setDstSubpass(lastPortalSubpassIdx)
				.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
				.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
			);

			}
		}
	}


	vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(GetSizeUint32(attachmentsDescritpions)).setPAttachments(std::data(attachmentsDescritpions))
		.setSubpassCount(GetSizeUint32(subpasses)).setPSubpasses(std::data(subpasses))
		.setDependencyCount(GetSizeUint32(dependencies)).setPDependencies(std::data(dependencies))
		;

	return logicalDevice.createRenderPassUnique(renderPassCreateInfo);

}




































/////////////// UNUSED

vk::UniqueRenderPass Renderpass::Initial_With_Portals(vk::Device logicalDevice)
{

	// 0. Render scene, with depth buffer
	// 1. Render Portals, with depth buffer, write RenderedDepth, write Stencil
	vk::Format colorFormat = {};
	vk::Format depthFormat = {};
	vk::Format renderedDepthFormat = vk::Format::eD32Sfloat;

	vk::ImageLayout finalColorLayout = vk::ImageLayout::eColorAttachmentOptimal;

	enum AttachmentIdx
	{
		colorAttachmentIdx_renderScene = 0,
		depthStencilAttachmentIdx_renderScene,
		colorAttachmentIdx_renderPortals,
		depthStencilAttachmentIdx_renderPortals,
		renderDepthAttachmentIdx_renderPortals,
		enum_size_attachmentIdx
	};

	std::array<vk::AttachmentDescription, AttachmentIdx::enum_size_attachmentIdx> attachments;
	attachments[colorAttachmentIdx_renderScene] = vk::AttachmentDescription()
		.setFormat(colorFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
		;

	attachments[depthStencilAttachmentIdx_renderScene] = vk::AttachmentDescription()
		.setFormat(depthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore) // We want to reuse it in the next subpass only, how do we keep it?
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		;

	//////////////////////// 
	attachments[colorAttachmentIdx_renderPortals] = vk::AttachmentDescription()
		.setFormat(colorFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setFinalLayout(finalColorLayout)
		;

	attachments[depthStencilAttachmentIdx_renderPortals] = vk::AttachmentDescription()
		.setFormat(depthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad) // use previous depth values
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)  // depth not needed anymore
		.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
		.setStencilStoreOp(vk::AttachmentStoreOp::eStore) // we need stencil it for later renderpasses
		.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		;


	// Stores the nearest just rendered depth, only needs to be written for fragments at portals,
	// it is used to dicard fragments that would lie in front of portals
	attachments[renderDepthAttachmentIdx_renderPortals] = vk::AttachmentDescription{}
		.setFormat(renderedDepthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore) // We want to keep this until rendering is completely finished
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
		;


	vk::AttachmentReference colorAttachmentRef_renderScene(colorAttachmentIdx_renderScene, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthAttachmentRef_renderScene(depthStencilAttachmentIdx_renderScene, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	enum SubpassDescriptionIndex
	{
		subpassIdx_renderScene = 0,
		subpassIdx_renderPortal,
		enum_size_subpassDescription
	};

	std::array<vk::SubpassDescription, SubpassDescriptionIndex::enum_size_subpassDescription> subpassDescriptions;

	subpassDescriptions[subpassIdx_renderScene] = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&colorAttachmentRef_renderScene)
		.setPDepthStencilAttachment(&depthAttachmentRef_renderScene)
		;

	vk::AttachmentReference colorAttachmentRef_renderPortals(colorAttachmentIdx_renderPortals, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthAttachmentRef_renderPortals(depthStencilAttachmentIdx_renderPortals, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::AttachmentReference renderDepthAttachmentRef_renderPortals(renderDepthAttachmentIdx_renderPortals, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentReference renderPortalColorRefs[] = { colorAttachmentRef_renderPortals, depthAttachmentRef_renderPortals };


	subpassDescriptions[subpassIdx_renderPortal] = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(GetSizeUint32(renderPortalColorRefs)).setPColorAttachments(renderPortalColorRefs)
		.setPDepthStencilAttachment(&renderDepthAttachmentRef_renderPortals);

	vk::SubpassDependency subpassDependency_External_0 = vk::SubpassDependency{}
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(subpassIdx_renderScene)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

	vk::SubpassDependency subpassDependency_0_1 = vk::SubpassDependency{}
		.setSrcSubpass(subpassIdx_renderScene)
		.setDstSubpass(subpassIdx_renderPortal)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

	vk::SubpassDependency dependencies[] = { subpassDependency_External_0, subpassDependency_0_1 };

	vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(GetSizeUint32(attachments)).setPAttachments(attachments.data())
		.setSubpassCount(GetSizeUint32(subpassDescriptions)).setPSubpasses(subpassDescriptions.data())
		.setDependencyCount(GetSizeUint32(dependencies)).setPDependencies(dependencies)
		;

	return logicalDevice.createRenderPassUnique(renderPassCreateInfo);
}

vk::UniqueRenderPass Renderpass::Recursion_With_Portals(vk::Device logicalDevice)
{
	// 0. Render Scene, using depth buffer, stencil buffer and RenderedDepth,	
	// 1. Render Portals, using depth buffer, stencil buffer and renderedDepth, set stencil buffer to portal Id + Recursion IDX

	vk::Format colorFormat = {};
	vk::Format depthFormat = {};
	vk::Format renderedDepthFormat = vk::Format::eD32Sfloat;

	vk::ImageLayout finalColorLayout = vk::ImageLayout::eColorAttachmentOptimal;

	enum AttachmentIdx
	{
		colorAttachmentIdx_renderScene = 0,
		depthStencilAttachmentIdx_renderScene,
		renderDepthAttachmentIdx_renderScene,
		colorAttachmentIdx_renderPortals,
		depthStencilAttachmentIdx_renderPortals,
		renderDepthAttachmentIdx_renderPortals,
		enum_size_attachmentIdx
	};

	std::array<vk::AttachmentDescription, AttachmentIdx::enum_size_attachmentIdx> attachments;
	attachments[colorAttachmentIdx_renderScene] = vk::AttachmentDescription()
		.setFormat(colorFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
		;

	attachments[depthStencilAttachmentIdx_renderScene] = vk::AttachmentDescription()
		.setFormat(depthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear) // we are rendering from a new portal position, depth buffer should be cleared
		.setStoreOp(vk::AttachmentStoreOp::eStore) // We want to reuse it in the next subpass only
		.setStencilLoadOp(vk::AttachmentLoadOp::eLoad) // we only want to render inside portal area and we need the stencil buffer for that
		.setStencilStoreOp(vk::AttachmentStoreOp::eStore) // we want to keep it, although we don't change stencil values
		.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		;

	// This is used as additional test to discard fragments in fron of the portal, we only want to draw stuff behind the portal
	attachments[renderDepthAttachmentIdx_renderScene] = vk::AttachmentDescription()
		.setFormat(depthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad)
		.setStoreOp(vk::AttachmentStoreOp::eStore) // we need to keep this
		.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
		;

	//////////////////////// 
	attachments[colorAttachmentIdx_renderPortals] = vk::AttachmentDescription()
		.setFormat(colorFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
		;

	attachments[depthStencilAttachmentIdx_renderPortals] = vk::AttachmentDescription()
		.setFormat(depthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad) // use previous depth values
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)  // depth not needed anymore
		.setStencilLoadOp(vk::AttachmentLoadOp::eLoad) // we never want to clear the stencil buffer
		.setStencilStoreOp(vk::AttachmentStoreOp::eStore) // we need stencil it for later renderpasses, again
		.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		;


	// Stores the nearest just rendered depth, only needs to be written for fragments at portals,
	// it is used to dicard fragments that would lie in front of portals
	attachments[renderDepthAttachmentIdx_renderPortals] = vk::AttachmentDescription{}
		.setFormat(renderedDepthFormat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eLoad)
		.setStoreOp(vk::AttachmentStoreOp::eStore) // We want to keep this until rendering is completely finished
		.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
		;


	vk::AttachmentReference colorAttachmentRef_renderScene(colorAttachmentIdx_renderScene, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthAttachmentRef_renderScene(depthStencilAttachmentIdx_renderScene, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::AttachmentReference renderedDepthAttachmentRef_renderScene(renderDepthAttachmentIdx_renderScene, vk::ImageLayout::eColorAttachmentOptimal);

	enum SubpassDescriptionIndex
	{
		subpassIdx_renderScene = 0,
		subpassIdx_renderPortal,
		enum_size_subpassDescription
	};

	std::array<vk::SubpassDescription, SubpassDescriptionIndex::enum_size_subpassDescription> subpassDescriptions;

	vk::AttachmentReference colorattachments_renderScene[] = { colorAttachmentRef_renderScene, depthAttachmentRef_renderScene };
	subpassDescriptions[subpassIdx_renderScene] = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(GetSizeUint32(colorattachments_renderScene)).setPColorAttachments(colorattachments_renderScene)
		.setPDepthStencilAttachment(&depthAttachmentRef_renderScene)
		;

	vk::AttachmentReference colorAttachmentRef_renderPortals(colorAttachmentIdx_renderPortals, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthAttachmentRef_renderPortals(depthStencilAttachmentIdx_renderPortals, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::AttachmentReference renderDepthAttachmentRef_renderPortals(renderDepthAttachmentIdx_renderPortals, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentReference renderPortalColorRefs[] = { colorAttachmentRef_renderPortals, depthAttachmentRef_renderPortals };


	subpassDescriptions[subpassIdx_renderPortal] = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(GetSizeUint32(renderPortalColorRefs)).setPColorAttachments(renderPortalColorRefs)
		.setPDepthStencilAttachment(&renderDepthAttachmentRef_renderPortals);

	vk::SubpassDependency subpassDependency_External_0 = vk::SubpassDependency{}
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(subpassIdx_renderScene)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

	vk::SubpassDependency subpassDependency_0_1 = vk::SubpassDependency{}
		.setSrcSubpass(subpassIdx_renderScene)
		.setDstSubpass(subpassIdx_renderPortal)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

	vk::SubpassDependency dependencies[] = { subpassDependency_External_0, subpassDependency_0_1 };

	vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(GetSizeUint32(attachments)).setPAttachments(attachments.data())
		.setSubpassCount(GetSizeUint32(subpassDescriptions)).setPSubpasses(subpassDescriptions.data())
		.setDependencyCount(GetSizeUint32(dependencies)).setPDependencies(dependencies)
		;

	return logicalDevice.createRenderPassUnique(renderPassCreateInfo);

}
