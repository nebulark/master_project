#include "pch.hpp"
#include "Renderpass.hpp"
#include "GetSizeUint32.hpp"
#include "NTree.hpp"


vk::UniqueRenderPass Renderpass::Portals_One_Pass_dynamicState(vk::Device logicalDevice, vk::Format colorFormat, 
	vk::Format depthStencilFormat, vk::Format renderedDepthFormat, vk::Format renderedStencilFormat, int iterationCount, std::vector<std::string>* optionalDebug /*= nullptr*/)
{
	enum AttachmentDescriptionIdx
	{
		color,
		depthStencil,
		renderedDepth_0,
		renderedDepth_1,	
		renderedStencil_0,
		renderedStencil_1,

		enum_size_AttachmentDescriptionIdx
	};

	constexpr vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;

	const std::array<vk::AttachmentDescription, AttachmentDescriptionIdx::enum_size_AttachmentDescriptionIdx> attachmentsDescritpions = 
	[samples, colorFormat, depthStencilFormat, renderedDepthFormat, renderedStencilFormat]()
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

		attachmentsDescritpions[renderedStencil_0] = vk::AttachmentDescription()
			.setFormat(renderedStencilFormat)
			.setSamples(samples)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
			;

		attachmentsDescritpions[renderedStencil_1] = attachmentsDescritpions[renderedStencil_0];

		return attachmentsDescritpions;
	}();


	//////////////////////////////////////////////////////////////////////////
	// Subpass Description



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

	const std::array<std::array<vk::AttachmentReference,3>, enum_size_IterationParity>  renderPortalOutput = []()
	{
		std::array<std::array<vk::AttachmentReference,3>, enum_size_IterationParity> renderPortalOutput;
		renderPortalOutput[even] = 
		{
			vk::AttachmentReference(renderedDepth_0, vk::ImageLayout::eColorAttachmentOptimal),
			vk::AttachmentReference(color, vk::ImageLayout::eColorAttachmentOptimal), // TODO: Put this into preserve attachments, for now we keep it for debugging
			vk::AttachmentReference(renderedStencil_0, vk::ImageLayout::eColorAttachmentOptimal),
		};
	
		renderPortalOutput[odd] = 
		{
			vk::AttachmentReference(renderedDepth_1, vk::ImageLayout::eColorAttachmentOptimal),
			vk::AttachmentReference(color, vk::ImageLayout::eColorAttachmentOptimal), // TODO: Put this into preserve attachments, for now we keep it for debugging
			vk::AttachmentReference(renderedStencil_1, vk::ImageLayout::eColorAttachmentOptimal),
		};


		return renderPortalOutput;
	}();

	// input attachments all subsequent passes will use (scene AND portal), take care that renderedDepth is the opposite of renderPortalOutput attachments!
	const std::array<std::array<vk::AttachmentReference,2>, enum_size_IterationParity> subsequentPassInput = []()
	{
		std::array<std::array<vk::AttachmentReference,2>, enum_size_IterationParity> subsequentPassInput;

		subsequentPassInput[even] = {
			vk::AttachmentReference(renderedDepth_1, vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::AttachmentReference(renderedStencil_1, vk::ImageLayout::eShaderReadOnlyOptimal),
		};

		subsequentPassInput[odd] = {
			vk::AttachmentReference(renderedDepth_0, vk::ImageLayout::eShaderReadOnlyOptimal),
			vk::AttachmentReference(renderedStencil_0, vk::ImageLayout::eShaderReadOnlyOptimal),
		};

		return subsequentPassInput;
	}();


	const uint32_t subpassCount = ((iterationCount + 1)* 2);
	std::unique_ptr<vk::SubpassDescription[]> subpassStorage = std::make_unique<vk::SubpassDescription[]>(subpassCount);
	gsl::span<vk::SubpassDescription> subpasses = gsl::make_span(subpassStorage.get(), subpassCount);


	// don't know how many there are, maybe test it?
	std::vector<vk::SubpassDependency> dependencies;


	// initial subpasses
	// intial scene subpass
	// iteration 0, this means use even where neccessary
	{
		if (optionalDebug)
		{
			optionalDebug->clear();
			optionalDebug->resize(std::size(subpasses));
		}

		constexpr int initialSceneSubpassIdx = 0;
		subpasses[initialSceneSubpassIdx] = vk::SubpassDescription{}
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(GetSizeUint32(renderSceneOutput)).setPColorAttachments(renderSceneOutput)
			.setPDepthStencilAttachment(&depthStencilAttachment)
			;

		if (optionalDebug)
		{
			(*optionalDebug)[initialSceneSubpassIdx] = "initialScene";
		}

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

		if (optionalDebug)
		{
			(*optionalDebug)[initialPortalSubpassIdx] = "initialPortal";
		}

		// we need to wait for the depth buffer write (and maybe color) before rendering portal
		dependencies.push_back(vk::SubpassDependency{}
			.setSrcSubpass(initialSceneSubpassIdx)
			.setDstSubpass(initialPortalSubpassIdx)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
			.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		);

		// self dependency to wait for writes to portal index helper
		dependencies.push_back(vk::SubpassDependency{}
			.setSrcSubpass(initialPortalSubpassIdx)
			.setDstSubpass(initialPortalSubpassIdx)
			.setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
			.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		);
	}

	// subsequent subpasses
	for (int iteration = 1; iteration <= iterationCount; ++iteration)
	{

		const IterationParity iterationParity = iteration % 2 == 0 ? even : odd;
		const int sceneSubpassIdx = iteration * 2;
		const int portalSubpassIdx = sceneSubpassIdx + 1;
		const int previousPortalSubpassIdx = sceneSubpassIdx - 1;

		subpasses[sceneSubpassIdx] = vk::SubpassDescription{}
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(GetSizeUint32(renderSceneOutput)).setPColorAttachments(renderSceneOutput)
			.setInputAttachmentCount(GetSizeUint32(subsequentPassInput[iterationParity]))
			.setPInputAttachments(std::data(subsequentPassInput[iterationParity]))
			.setPDepthStencilAttachment(&depthStencilAttachment)
			;

		if (optionalDebug)
		{
			char buff[128] = {};
			int bytesWritten = 128;

			bytesWritten = std::sprintf(buff, "scene l%i o c, %s", iteration, (iterationParity == even ? "i rd1" : "i rd0"));
			assert(bytesWritten < std::size(buff));

			(*optionalDebug)[sceneSubpassIdx] = buff;
		}

		// we depend on the last portal pass from the previous layer, which is responsible for clearing depth
		dependencies.push_back(vk::SubpassDependency{}
			.setSrcSubpass(previousPortalSubpassIdx)
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

		if (optionalDebug)
		{
			char buff[128] = {};
			int bytesWritten = 128;

			bytesWritten = std::sprintf(buff, "portal l%i %s %s", iteration, (iterationParity == even ? "o rd0" : "o rd1"), (iterationParity == even ? "i rd1" : "i rd0"));
			assert(bytesWritten < std::size(buff));

			(*optionalDebug)[portalSubpassIdx] = buff;
		}

		// we need to wait for the depth buffer write (and maybe color) before rendering portal
		dependencies.push_back(vk::SubpassDependency{}
			.setSrcSubpass(sceneSubpassIdx)
			.setDstSubpass(portalSubpassIdx)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
			.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		);	

		// self dependency to wait for writes to portal index helper
		dependencies.push_back(vk::SubpassDependency{}
			.setSrcSubpass(portalSubpassIdx)
			.setDstSubpass(portalSubpassIdx)
			.setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
			.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		);

	}

	vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(GetSizeUint32(attachmentsDescritpions)).setPAttachments(std::data(attachmentsDescritpions))
		.setSubpassCount(GetSizeUint32(subpasses)).setPSubpasses(std::data(subpasses))
		.setDependencyCount(GetSizeUint32(dependencies)).setPDependencies(std::data(dependencies))
		;

	return logicalDevice.createRenderPassUnique(renderPassCreateInfo);
}
