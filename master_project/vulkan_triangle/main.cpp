// raytrace_software.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include "common/Deleters.hpp"
#include "common/VulkanDebug.hpp"
#include "common/VulkanDevice.hpp"
#include "common/VulkanUtils.hpp"
#include "common/FileHelper.hpp"


vk::UniqueInstance create_vulkan_instance(gsl::span<const char*> enabledLayers, gsl::span<const char*> enabledExtensions)
{
	vk::ApplicationInfo application_info("Hello Triangle", 1, "My Engine", 1, VK_API_VERSION_1_1);


	vk::InstanceCreateInfo instanceCreateInfo({}, &application_info,
		static_cast<uint32_t>(std::size(enabledLayers)), std::data(enabledLayers),
		static_cast<uint32_t>(std::size(enabledExtensions)), std::data(enabledExtensions));


	vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfo);

		
	
	return instance;
}

std::vector<const char*> GetSdlExtensions(SDL_Window* window)
{
	unsigned int sdl_extension_count = -1;
	{
		const bool success = SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, nullptr);
	}

	std::vector<const char*> enabledExtensions;
	enabledExtensions.resize(sdl_extension_count);
	{
		const bool success = SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, enabledExtensions.data());
		assert(success);
	}

	return enabledExtensions;
}

// Instance must live longer than the returned object!!!
vk::UniqueSurfaceKHR CreateSurface(vk::Instance& instance, SDL_Window* window)
{

	VkSurfaceKHR vksurface = nullptr;
	const bool success = SDL_Vulkan_CreateSurface(window, instance, &vksurface);
	assert(success);

	return vk::UniqueSurfaceKHR(
		vk::SurfaceKHR(vksurface),
		vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderStatic>(instance)
	);
}


VkBool32 DebugUtilsCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	vk::DebugUtilsMessageSeverityFlagBitsEXT severity = static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity);
	vk::DebugUtilsMessageTypeFlagsEXT types(messageTypes);

	std::cerr << vk::to_string(severity) << ' ' << vk::to_string(types) << ' ' << pCallbackData->pMessage << '\n';
	
	constexpr VkBool32 shouldAbortFunctionCall = VK_TRUE;
	return shouldAbortFunctionCall;
}

vk::UniqueDebugUtilsMessengerEXT CreateDebugUtilsMessenger(vk::Instance instance)
{
	vk::DebugUtilsMessageSeverityFlagsEXT severityFlags = vk::DebugUtilsMessageSeverityFlagsEXT()
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
		//| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
		;

	vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags = vk::DebugUtilsMessageTypeFlagsEXT()
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
		;

	vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info(vk::DebugUtilsMessengerCreateFlagsEXT(), severityFlags, messageTypeFlags, &DebugUtilsCallback);
	vk::UniqueDebugUtilsMessengerEXT messenger = instance.createDebugUtilsMessengerEXTUnique(debug_utils_create_info);

	return messenger;
}

vk::UniqueImageView CreateImageView(vk::Image image, vk::Format surfaceFormat, vk::Device logicalDevice)
{
	const vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo()
		.setImage(image)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(surfaceFormat)
		.setComponents(vk::ComponentMapping()
			.setR(vk::ComponentSwizzle::eIdentity)
			.setG(vk::ComponentSwizzle::eIdentity)
			.setB(vk::ComponentSwizzle::eIdentity)
			.setA(vk::ComponentSwizzle::eIdentity))
		.setSubresourceRange(vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1))
		;

	return logicalDevice.createImageViewUnique(imageViewCreateInfo);
}

vk::UniqueShaderModule CreateShaderModule(gsl::span<const char> shaderCode, vk::Device logicalDevice)
{
	assert(reinterpret_cast<std::uintptr_t>(std::data(shaderCode)) % alignof(uint32_t) == 0);

	vk::ShaderModuleCreateInfo createInfo(vk::ShaderModuleCreateFlags{},
		std::size(shaderCode), reinterpret_cast<const uint32_t*>(std::data(shaderCode)));

	return logicalDevice.createShaderModuleUnique(createInfo);
}


int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	{
		constexpr int width = 1920 / 2;
		constexpr int height = 1080 / 2;

		constexpr int maxFramesInFlight = 2;


		WindowPtr window{
			SDL_CreateWindow(
			"An SDL2 window",                  // window title
			SDL_WINDOWPOS_UNDEFINED,           // initial x position
			SDL_WINDOWPOS_UNDEFINED,           // initial y position
			width,                               // width, in pixels
			height,                               // height, in pixels
			SDL_WINDOW_VULKAN                 // flags - see below
			)
		};


		const char* enabledValidationLayers[] =
		{
			// Enable standard validation layer to find as much errors as possible!
			"VK_LAYER_KHRONOS_validation"
		};

		std::vector<const char*> enabledExtensions = GetSdlExtensions(window.get());
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);


		assert(VulkanUtils::SupportsValidationLayers(enabledValidationLayers));
		vk::UniqueInstance instance = create_vulkan_instance(enabledValidationLayers, enabledExtensions);


		VulkanDebugExtension::LoadDebugUtilsMessengerExtension(instance.get());

		vk::UniqueDebugUtilsMessengerEXT messenger = CreateDebugUtilsMessenger(instance.get());

		vk::UniqueSurfaceKHR surface = CreateSurface(instance.get(), window.get());

		const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VulkanDevice::QueueRequirement queueRequirement[1];
		queueRequirement[0].canPresent = true;
		queueRequirement[0].mincount = 1;
		queueRequirement[0].minFlags = vk::QueueFlagBits::eGraphics;

		VulkanDevice::DeviceRequirements deviceRequirements;
		deviceRequirements.queueRequirements = queueRequirement;
		deviceRequirements.requiredExtensions = deviceExtensions;

		std::optional<VulkanDevice::PickDeviceResult> maybeDeviceResult =
			VulkanDevice::PickPhysicalDevice(instance.get(), deviceRequirements, &(surface.get()));

		assert(maybeDeviceResult);
		VulkanDevice::PickDeviceResult deviceResult = std::move(*maybeDeviceResult);

		vk::UniqueDevice logicalDevice = VulkanDevice::CreateLogicalDevice(
			deviceResult.device, deviceResult.queueResult, enabledValidationLayers, deviceExtensions);

		const vk::SurfaceFormatKHR preferedFormats[] = {
			vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
		};

		const vk::SurfaceFormatKHR swapchainFormat = VulkanUtils::ChooseSurfaceFormat(
			deviceResult.device.getSurfaceFormatsKHR(surface.get()),
			preferedFormats);

		const vk::PresentModeKHR preferedPresentationModes[] = { vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate };

		const vk::PresentModeKHR presentMode = VulkanUtils::ChoosePresentMode(
			deviceResult.device.getSurfacePresentModesKHR(surface.get()),
			preferedPresentationModes
		);

		const vk::SurfaceCapabilitiesKHR capabilities = deviceResult.device.getSurfaceCapabilitiesKHR(surface.get());
		const vk::Extent2D swapchainExtent = VulkanUtils::ChooseExtent(
			capabilities,
			vk::Extent2D(width, height)
		);

		const uint32_t imageCount = VulkanUtils::ChooseImageCount(capabilities, capabilities.minImageCount + 1);

		const vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
			.setSurface(surface.get())
			.setMinImageCount(imageCount)
			.setImageFormat(swapchainFormat.format)
			.setImageColorSpace(swapchainFormat.colorSpace)
			.setImageExtent(swapchainExtent)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setImageSharingMode(vk::SharingMode::eExclusive) // we only have one queue
			.setPreTransform(capabilities.currentTransform)
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setPresentMode(presentMode)
			.setClipped(true)
			.setOldSwapchain(vk::SwapchainKHR());

		vk::UniqueHandle<vk::SwapchainKHR, vk::DispatchLoaderStatic> swapChain =
			logicalDevice->createSwapchainKHRUnique(swapchainCreateInfo);

		std::vector<vk::Image> swapchainImages = logicalDevice->getSwapchainImagesKHR(swapChain.get());

		std::vector<vk::UniqueImageView> swapchainImageViews;
		swapchainImageViews.reserve(swapchainImages.size());

		for (const vk::Image& image : swapchainImages)
		{
			swapchainImageViews.push_back(CreateImageView(image, swapchainFormat.format, logicalDevice.get()));
		}
		
		//VulkanDevice::LogicalDevice logicalDevice = VulkanDevice::(instance.get(), enabledValidationLayers, deviceExtensions, surface.get());
		

		// render pass

		vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
			.setFormat(swapchainFormat.format)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
			;

		// has something to do with frag shader layout 0
		vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
		vk::SubpassDescription subpassDescription = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(1)
			.setPColorAttachments(&colorAttachmentRef);

		vk::SubpassDependency subpassDependency = vk::SubpassDependency{}
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlags())
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);


		vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
			.setAttachmentCount(1).setPAttachments(&colorAttachment)
			.setSubpassCount(1).setPSubpasses(&subpassDescription)
			.setDependencyCount(1).setPDependencies(&subpassDependency);

		vk::UniqueHandle<vk::RenderPass, vk::DispatchLoaderStatic> renderpass 
			= logicalDevice->createRenderPassUnique(renderPassCreateInfo);


		// graphics pipeline

		std::vector<char> vertShaderCode = FileHelper::LoadFileContent("triangle.vert.spv");
		std::vector<char> fragShaderCode = FileHelper::LoadFileContent("triangle.frag.spv");

		vk::UniqueShaderModule vertShaderModule = CreateShaderModule(vertShaderCode, logicalDevice.get());
		vk::UniqueShaderModule fragShaderModule = CreateShaderModule(fragShaderCode, logicalDevice.get());

		vk::PipelineShaderStageCreateInfo pipelineShaderStageCreationInfos[] =
		{
			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eVertex,
				vertShaderModule.get(),
				"main"),

			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eFragment,
				fragShaderModule.get(),
				"main"),
		};

		// same for frag

		// fill out when we use vertex data
		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};

		// true enables breaking up strips (line, triangels) with special indices   when using drawing with indices 
		constexpr bool primitiveRestartEnable = false;
		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
			vk::PipelineInputAssemblyStateCreateFlags(),
			vk::PrimitiveTopology::eTriangleList,
			primitiveRestartEnable
		);

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
			.setFrontFace(vk::FrontFace::eClockwise)
			.setDepthBiasEnable(false)
			.setDepthBiasConstantFactor(0.f)
			.setDepthBiasClamp(0.f)
			.setDepthBiasSlopeFactor(0.f);

		vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
			.setSampleShadingEnable(false)
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			.setMinSampleShading(1.f)
			.setPSampleMask(nullptr)
			.setAlphaToCoverageEnable(false)
			.setAlphaToOneEnable(false);

		//vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo();

		vk::PipelineColorBlendAttachmentState colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
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

		vk::PipelineColorBlendStateCreateInfo colorBlendingStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(false)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(1)
			.setPAttachments(&colorBlendAttachment)
			.setBlendConstants({ 0.f,0.f,0.f,0.f })
			;

		// useful for uniforms
		vk::PipelineLayoutCreateInfo pipelineLayoutcreateInfo = vk::PipelineLayoutCreateInfo(
			vk::PipelineLayoutCreateFlags(),
			0, nullptr,
			0, nullptr);

		vk::UniqueHandle<vk::PipelineLayout, vk::DispatchLoaderStatic> pipelineLayout =
			logicalDevice->createPipelineLayoutUnique(pipelineLayoutcreateInfo);

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
			vk::PipelineCreateFlags(), //| vk::PipelineCreateFlagBits::eDerivative,
			gsl::narrow<uint32_t>(std::size(pipelineShaderStageCreationInfos)), std::data(pipelineShaderStageCreationInfos),
			&pipelineVertexInputStateCreateInfo,
			&pipelineInputAssemblyStateCreateInfo,
			nullptr, // tesselationstate
			&viewportStateCreateInfo,
			&rasterizationStateCreateInfo,
			&multisampleStateCreateInfo,
			nullptr, // PipelineDepthStencilStateCreateInfo
			&colorBlendingStateCreateInfo,
			nullptr, // dynamic device state
			pipelineLayout.get(),
			renderpass.get(),
			0,
			vk::Pipeline(),
			-1);

		vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> graphicsPipeline =
			logicalDevice->createGraphicsPipelineUnique(vk::PipelineCache(), graphicsPipelineCreateInfo);


		std::vector<vk::UniqueFramebuffer> swapChainFramebuffers;
		swapChainFramebuffers.reserve(swapchainImageViews.size());
		for (vk::UniqueImageView& imageView : swapchainImageViews)
		{
			vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo{}
				.setRenderPass(renderpass.get())
				.setAttachmentCount(1).setPAttachments(&(imageView.get()))
				.setWidth(swapchainExtent.width)
				.setHeight(swapchainExtent.height)
				.setLayers(1);

			swapChainFramebuffers.push_back(logicalDevice->createFramebufferUnique(framebufferCreateInfo));
		}

		// command buffers

		vk::CommandPoolCreateInfo commandPoolCreateInfo(
			vk::CommandPoolCreateFlags() /*there are possible flags*/,
			deviceResult.queueResult[0].familyIndex);
		vk::UniqueCommandPool commandPool = logicalDevice->createCommandPoolUnique(commandPoolCreateInfo);

		vk::CommandBufferAllocateInfo commandBufferAllocInfo(
			commandPool.get(), vk::CommandBufferLevel::ePrimary, gsl::narrow<uint32_t>(swapChainFramebuffers.size()));

		std::vector<vk::UniqueCommandBuffer> commandBuffers = logicalDevice->allocateCommandBuffersUnique(commandBufferAllocInfo);
		for (size_t i = 0, count = commandBuffers.size(); i < count; ++i)
		{
			vk::CommandBufferBeginInfo commandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr);
			commandBuffers[i]->begin(commandBufferBeginInfo);

			vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }));
			vk::RenderPassBeginInfo renderpassBeginInfo = vk::RenderPassBeginInfo{}
				.setRenderPass(renderpass.get())
				.setFramebuffer(swapChainFramebuffers[i].get())
				.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent))
				.setClearValueCount(1).setPClearValues(&clearColor);

			commandBuffers[i]->beginRenderPass(renderpassBeginInfo, vk::SubpassContents::eInline);

			commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.get());
			commandBuffers[i]->draw(3, 1, 0, 0);
			commandBuffers[i]->endRenderPass();
			commandBuffers[i]->end();
		}

		vk::Queue graphicsPresentQueue = logicalDevice->getQueue(deviceResult.queueResult[0].familyIndex, 0);
		//

		// semaphores

		std::array< vk::UniqueSemaphore, maxFramesInFlight> imageAvailableSemaphores;
		std::array< vk::UniqueSemaphore, maxFramesInFlight> renderFinishedSemaphores;
		for (size_t i = 0; i < maxFramesInFlight; ++i)
		{
			imageAvailableSemaphores[i] = logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
			renderFinishedSemaphores[i] = logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		}

		int currentFrame = 0;

		
		while (true)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (SDL_WINDOWEVENT == event.type && SDL_WINDOWEVENT_CLOSE == event.window.event)
				{
					goto endMainLoop;
				}
				//handle_event(event);
			}

			// draw


			constexpr uint64_t noTimeout = std::numeric_limits<uint64_t>::max();
			uint32_t imageIndex = logicalDevice->acquireNextImageKHR(
				swapChain.get(), noTimeout, imageAvailableSemaphores[currentFrame].get(), vk::Fence()).value;

			vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
			vk::SubmitInfo submitInfo = vk::SubmitInfo{}
				.setWaitSemaphoreCount(1).setPWaitSemaphores(&(imageAvailableSemaphores[currentFrame].get())).setPWaitDstStageMask(waitStages)
				.setCommandBufferCount(1).setPCommandBuffers(&(commandBuffers[imageIndex].get()))
				.setSignalSemaphoreCount(1).setPSignalSemaphores(&(renderFinishedSemaphores[currentFrame].get()));

			graphicsPresentQueue.submit(1, &submitInfo, vk::Fence());

			vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR{}
				.setWaitSemaphoreCount(1).setPWaitSemaphores(&(renderFinishedSemaphores[currentFrame].get()))
				.setSwapchainCount(1).setPSwapchains(&(swapChain.get())).setPImageIndices(&imageIndex);

			graphicsPresentQueue.presentKHR(presentInfo);

			
			SDL_UpdateWindowSurface(window.get());

			currentFrame = (currentFrame + 1) % maxFramesInFlight;
		}

	endMainLoop:;
		logicalDevice->waitIdle();
		
		
	}
	SDL_Quit();
		

	return 0;
	
}


