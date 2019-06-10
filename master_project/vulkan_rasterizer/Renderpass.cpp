#include "pch.hpp"
#include "Renderpass.hpp"
#include "GetSizeUint32.hpp"




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

vk::UniqueRenderPass Renderpass::Initial_With_Portals(vk::Device logicalDevice)
{
	
	// 0. Render scene, with depth buffer
	// 1. Render Portals, with depth buffer, write RenderedDepth, write Stencil
	vk::Format colorFormat;
	vk::Format depthFormat;
	vk::Format renderedDepthFormat = vk::Format::eD32Sfloat;

	vk::ImageLayout finalColorLayout = vk::ImageLayout::eColorAttachmentOptimal;

	enum AttachmentIdx
	{
		colorAttachmentIdx_renderScene = 0,
		depthStencilAttachmentIdx_renderScene,
		colorAttachmentIdx_renderPortals,
		depthStencilAttachmentIdx_renderPortals,
		renderDepthAttachmentIdx_renderPortals,
		enum_size
	};

	std::array<vk::AttachmentDescription, AttachmentIdx::enum_size> attachments;
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
		enum_size
	};

	std::array<vk::SubpassDescription, SubpassDescriptionIndex::enum_size> subpassDescriptions;

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

	vk::Format colorFormat;
	vk::Format depthFormat;
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
		enum_size
	};

	std::array<vk::AttachmentDescription, AttachmentIdx::enum_size> attachments;
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
		enum_size
	};

	std::array<vk::SubpassDescription, SubpassDescriptionIndex::enum_size> subpassDescriptions;

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
