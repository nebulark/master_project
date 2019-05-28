#include "pch.h"
#include "Application.hpp"
#include "common/VulkanUtils.hpp"
#include "common/FileHelper.hpp"
#include "common/VulkanDebug.hpp"
#include "common/VulkanDevice.hpp"
#include "common/SmallArraySet.hpp"

namespace
{
	template<typename T>
	constexpr vk::Format GlmTypeToVkFormat()
	{
		if constexpr (std::is_same_v<glm::vec1, T>) {
			return vk::Format::eR32Sfloat;
		}
		else if constexpr (std::is_same_v<glm::vec2, T>) {
			return vk::Format::eR32G32Sfloat;
		}
		else if constexpr (std::is_same_v<glm::vec3, T>) {
			return vk::Format::eR32G32B32Sfloat;
		}
		else if constexpr (std::is_same_v<glm::vec4, T>) {
			return vk::Format::eR32G32B32A32Sfloat;
		}
		else
		{
			static_assert(false, "Type not supported");
			return vk::Format;
		}
	}

	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 color;

		// maybe move this out, 
		static vk::VertexInputBindingDescription GetBindingDescription()
		{
			vk::VertexInputBindingDescription bindingDescription = vk::VertexInputBindingDescription{}
				.setBinding(0) // all data is interleave in one array
				.setStride(sizeof(Vertex))
				.setInputRate(vk::VertexInputRate::eVertex);

			return bindingDescription;
		}

		static std::array<vk::VertexInputAttributeDescription, 2> GetAttributeDescriptions() {
			std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0; // needs to match shader
			attributeDescriptions[0].format = GlmTypeToVkFormat<decltype(Vertex::pos)>();
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1; // needs to match shader
			attributeDescriptions[1].format = GlmTypeToVkFormat<decltype(Vertex::color)>();
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};

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

	vk::UniqueImageView CreateSimpleImageView2D(vk::Image image, vk::Format surfaceFormat, vk::Device logicalDevice)
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

	vk::UniqueHandle<vk::RenderPass, vk::DispatchLoaderStatic> CreateRenderpass(vk::Device logicalDevice, vk::Format swapchainFormat)
	{
		vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
			.setFormat(swapchainFormat)
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

		return logicalDevice.createRenderPassUnique(renderPassCreateInfo);
	}

	vk::UniqueHandle<vk::PipelineLayout, vk::DispatchLoaderStatic> CreatePipelineLayout(vk::Device logicalDevice)
	{
		// useful for uniforms
		vk::PipelineLayoutCreateInfo pipelineLayoutcreateInfo = vk::PipelineLayoutCreateInfo(
			vk::PipelineLayoutCreateFlags(),
			0, nullptr,
			0, nullptr);


		return logicalDevice.createPipelineLayoutUnique(pipelineLayoutcreateInfo);
	}

	vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> CreateGraphicsPipeline(
		vk::Device logicalDevice, vk::Extent2D swapchainExtent, vk::RenderPass renderpass,
		vk::PipelineLayout pipelineLayout, gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos)
	{
		// fill out when we use vertex data

		std::array<vk::VertexInputAttributeDescription,2> vertexAttributeDescription = Vertex::GetAttributeDescriptions();
		vk::VertexInputBindingDescription vertexBindingDescription = Vertex::GetBindingDescription();

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{}
			.setVertexBindingDescriptionCount(1)
			.setPVertexBindingDescriptions(&vertexBindingDescription)
			.setVertexAttributeDescriptionCount(gsl::narrow<uint32_t>(std::size(vertexAttributeDescription)))
			.setPVertexAttributeDescriptions(std::data(vertexAttributeDescription));

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
			pipelineLayout,
			renderpass,
			0,
			vk::Pipeline(),
			-1);

		return logicalDevice.createGraphicsPipelineUnique(vk::PipelineCache(), graphicsPipelineCreateInfo);
	}


	constexpr vk::SurfaceFormatKHR preferedSurfaceFormats[] = {
			vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
	};

	const vk::PresentModeKHR preferedPresentationModes[] = { vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate };


	struct PhysicalDeviceInfo
	{
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
		vk::SurfaceCapabilitiesKHR capabilities;
	};

	PhysicalDeviceInfo GetPhysicalDeviceInfo(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
	{
		return
		{
			physicalDevice.getSurfaceFormatsKHR(surface),
			physicalDevice.getSurfacePresentModesKHR(surface),
			physicalDevice.getSurfaceCapabilitiesKHR(surface)
		};
	}

	// suitableMemoryTypesBits - each bit corresponds to whether the device memory type with that index is suitable
	// if bit 0 is set than the physicalDeviceMemoryProperties.memoryTypes[0] is allowed
	uint32_t findMemoryTypeIdx(vk::PhysicalDevice physicalDevice, uint32_t suitableMemoryTypesBits, vk::MemoryPropertyFlags properties) {
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((suitableMemoryTypesBits & (1 << i))
				&& ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	SimpleBuffer createBuffer(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice,
		uint32_t size, vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags memoryProperties, gsl::span<const uint32_t> queueFamilyIndices = {})
	{
		vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo{}
			.setSize(size)
			.setUsage(usage)
			.setSharingMode(vk::SharingMode::eExclusive);

		// needs to be declared outside as the reference needs to be valid until creation
		SmallArraySet<uint32_t> uniqueQueuFamilies;
		if (std::size(queueFamilyIndices) > 1)
		{
			uniqueQueuFamilies.add_unique(std::begin(queueFamilyIndices), std::end(queueFamilyIndices));
			if (uniqueQueuFamilies.size() > 1)
			{
				bufferCreateInfo.setSharingMode(vk::SharingMode::eConcurrent)
					.setQueueFamilyIndexCount(gsl::narrow<uint32_t>(std::size(uniqueQueuFamilies)))
					.setPQueueFamilyIndices(std::data(uniqueQueuFamilies));
			}
		}

		vk::UniqueDeviceMemory bufferMemory;
		vk::UniqueBuffer buffer = logicalDevice.createBufferUnique(bufferCreateInfo);


		vk::MemoryRequirements vertexBufferMemoryRequirement = logicalDevice.getBufferMemoryRequirements(buffer.get());
		uint32_t vertexBufferMemoryTypeIdx = findMemoryTypeIdx(physicalDevice,
			vertexBufferMemoryRequirement.memoryTypeBits,
			memoryProperties);

		vk::MemoryAllocateInfo vertexBufferMemoryAllocInfo = vk::MemoryAllocateInfo{}
			.setAllocationSize(vertexBufferMemoryRequirement.size)
			.setMemoryTypeIndex(vertexBufferMemoryTypeIdx);

		bufferMemory = logicalDevice.allocateMemoryUnique(vertexBufferMemoryAllocInfo);
		logicalDevice.bindBufferMemory(buffer.get(), bufferMemory.get(), 0);

		return { std::move(bufferMemory), std::move(buffer), size };
	}

	void copyBuffer(vk::Device logicaldevice, vk::CommandPool copyCommandPool, vk::Queue transferQueue,
		vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) 
	{

		vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo{}
			.setCommandBufferCount(1)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandPool(copyCommandPool);


		vk::UniqueCommandBuffer copyCommandBuffer;
		{
			vk::CommandBuffer cb_temp;
			logicaldevice.allocateCommandBuffers(&allocInfo, &cb_temp);

			vk::PoolFree<vk::Device, vk::CommandPool, vk::DispatchLoaderStatic> deleter(
				logicaldevice, allocInfo.commandPool, vk::DispatchLoaderStatic());

			copyCommandBuffer = vk::UniqueCommandBuffer(cb_temp, deleter);
		}
		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		copyCommandBuffer->begin(beginInfo);
		vk::BufferCopy bufferCopy(0, 0, size);
		copyCommandBuffer->copyBuffer(srcBuffer, dstBuffer, bufferCopy);
		copyCommandBuffer->end();
		
		vk::SubmitInfo submitInfo = vk::SubmitInfo{}
			.setCommandBufferCount(1)
			.setPCommandBuffers(&(copyCommandBuffer.get()));

		transferQueue.submit(submitInfo, vk::Fence{});
		transferQueue.waitIdle();
	}

	const std::array<Vertex, 3> vertices = {
		Vertex{{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
		Vertex{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		Vertex{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};
}

Application::Application()
{	
	constexpr int width = 1920 / 2;
	constexpr int height = 1080 / 2;

	m_sdlWindow = WindowPtr {
		SDL_CreateWindow(
		"An SDL2 window",                
		SDL_WINDOWPOS_UNDEFINED,          
		SDL_WINDOWPOS_UNDEFINED,           
		width,                             
		height,                               
		SDL_WindowFlags::SDL_WINDOW_VULKAN | SDL_WindowFlags::SDL_WINDOW_RESIZABLE
		)
	};

	const char* enabledValidationLayers[] =
	{
		// Enable standard validation layer to find as much errors as possible!
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> enabledExtensions = GetSdlExtensions(m_sdlWindow.get());
	enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);


	assert(VulkanUtils::SupportsValidationLayers(enabledValidationLayers));
	m_vkInstance = create_vulkan_instance(enabledValidationLayers, enabledExtensions);


	VulkanDebugExtension::LoadDebugUtilsMessengerExtension(m_vkInstance.get());

	m_debugUtilsMessenger = CreateDebugUtilsMessenger(m_vkInstance.get());

	m_surface = CreateSurface(m_vkInstance.get(), m_sdlWindow.get());

	const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VulkanDevice::QueueRequirement queueRequirement[2];
	queueRequirement[0].canPresent = true;
	queueRequirement[0].mincount = 1;
	queueRequirement[0].minFlags = vk::QueueFlagBits::eGraphics;

	queueRequirement[1].canPresent = false;
	queueRequirement[1].mincount = 1;
	queueRequirement[1].minFlags = vk::QueueFlagBits::eTransfer;

	VulkanDevice::DeviceRequirements deviceRequirements;
	deviceRequirements.queueRequirements = queueRequirement;
	deviceRequirements.requiredExtensions = deviceExtensions;

	std::optional<VulkanDevice::PickDeviceResult> maybeDeviceResult =
		VulkanDevice::PickPhysicalDevice(m_vkInstance.get(), deviceRequirements, &(m_surface.get()));

	assert(maybeDeviceResult);
	m_physicalDevice = maybeDeviceResult->device;
	m_deviceQueueInfo = std::move(maybeDeviceResult->queueResult);


	m_logicalDevice = VulkanDevice::CreateLogicalDevice(
		m_physicalDevice, m_deviceQueueInfo, enabledValidationLayers, deviceExtensions);

	std::vector<char> vertShaderCode = FileHelper::LoadFileContent("triangle.vert.spv");
	std::vector<char> fragShaderCode = FileHelper::LoadFileContent("triangle.frag.spv");

	m_vertShaderModule = CreateShaderModule(vertShaderCode, m_logicalDevice.get());
	m_fragShaderModule = CreateShaderModule(fragShaderCode, m_logicalDevice.get());

	m_pipelineLayout = CreatePipelineLayout(m_logicalDevice.get());


	// command buffers

	vk::CommandPoolCreateInfo commandPoolCreateInfo_graphcisPresentQueue(
		vk::CommandPoolCreateFlags() /*there are possible flags*/,
		m_deviceQueueInfo[0].familyIndex);
	m_graphcisPresentQueueCommandPool = m_logicalDevice->createCommandPoolUnique(commandPoolCreateInfo_graphcisPresentQueue);

	vk::CommandPoolCreateInfo commandPoolCreateInfo_transferQueue(
		vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		m_deviceQueueInfo[1].familyIndex);
	m_transferQueueCommandPool = m_logicalDevice->createCommandPoolUnique(commandPoolCreateInfo_transferQueue);



	m_graphicsPresentQueue = m_logicalDevice->getQueue(m_deviceQueueInfo[0].familyIndex, m_deviceQueueInfo[0].offset);
	m_transferQueue = m_logicalDevice->getQueue(m_deviceQueueInfo[1].familyIndex, m_deviceQueueInfo[1].offset);
		
	for (size_t i = 0; i < maxFramesInFlight; ++i)
	{
		m_imageAvailableSemaphores[i] = m_logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		m_renderFinishedSemaphores[i] = m_logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		m_inFlightFences[i] = m_logicalDevice->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}

	m_currentFrame = 0;

	uint32_t familyIndices[] = { m_deviceQueueInfo[0].familyIndex, m_deviceQueueInfo[1].familyIndex };
	const vk::DeviceSize verticesByteSize = sizeof(vertices[0]) * std::size(vertices);
	m_vertexBuffer = createBuffer(
		m_logicalDevice.get(), m_physicalDevice,
		verticesByteSize,
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		familyIndices);

	SimpleBuffer stagingBuffer = createBuffer(
		m_logicalDevice.get(), m_physicalDevice, verticesByteSize, vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	{
		void* mappedVertexBufferMemory = m_logicalDevice->mapMemory(stagingBuffer.bufferMemory.get(), 0, stagingBuffer.size);
		std::memcpy(mappedVertexBufferMemory, std::data(vertices), stagingBuffer.size);
		m_logicalDevice->unmapMemory(stagingBuffer.bufferMemory.get());
	}

	copyBuffer(m_logicalDevice.get(), m_transferQueueCommandPool.get(), m_transferQueue,
		stagingBuffer.buffer.get(), m_vertexBuffer.buffer.get(), verticesByteSize);

	CreateSwapchain();
}

void Application::CreateSwapchain()
{
	PhysicalDeviceInfo physicalDeviceInfo = GetPhysicalDeviceInfo(m_physicalDevice, m_surface.get());

	const vk::SurfaceFormatKHR swapchainFormat = VulkanUtils::ChooseSurfaceFormat(
		m_physicalDevice.getSurfaceFormatsKHR(m_surface.get()),
		preferedSurfaceFormats);


	const vk::PresentModeKHR presentMode = VulkanUtils::ChoosePresentMode(
		m_physicalDevice.getSurfacePresentModesKHR(m_surface.get()),
		preferedPresentationModes
	);

	int currentWidth, currentHeight;
	SDL_GetWindowSize(m_sdlWindow.get(), &currentWidth, &currentHeight);

	const vk::SurfaceCapabilitiesKHR capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());
	const vk::Extent2D swapchainExtent = VulkanUtils::ChooseExtent(
		capabilities,
		vk::Extent2D(currentWidth, currentHeight)
	);

	const uint32_t imageCount = VulkanUtils::ChooseImageCount(capabilities, capabilities.minImageCount + 1);

	const vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
		.setSurface(m_surface.get())
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

	m_swapChain = m_logicalDevice->createSwapchainKHRUnique(swapchainCreateInfo);

	std::vector<vk::Image> swapchainImages = m_logicalDevice->getSwapchainImagesKHR(m_swapChain.get());

	m_renderpass = CreateRenderpass(m_logicalDevice.get(), swapchainFormat.format);

	vk::PipelineShaderStageCreateInfo pipelineShaderStageCreationInfos[] =
	{
		vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eVertex,
			m_vertShaderModule.get(),
			"main"),

		vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eFragment,
			m_fragShaderModule.get(),
			"main"),
	};

	m_graphicsPipeline = CreateGraphicsPipeline(
		m_logicalDevice.get(), swapchainExtent, m_renderpass.get(), m_pipelineLayout.get(), pipelineShaderStageCreationInfos);


	m_swapchainImageViews.reserve(swapchainImages.size());

	for (const vk::Image& image : swapchainImages)
	{
		m_swapchainImageViews.push_back(CreateSimpleImageView2D(image, swapchainFormat.format, m_logicalDevice.get()));
	}

	m_swapChainFramebuffers.reserve(m_swapchainImageViews.size());
	for (vk::UniqueImageView& imageView : m_swapchainImageViews)
	{
		vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo{}
			.setRenderPass(m_renderpass.get())
			.setAttachmentCount(1).setPAttachments(&(imageView.get()))
			.setWidth(swapchainExtent.width)
			.setHeight(swapchainExtent.height)
			.setLayers(1);

		m_swapChainFramebuffers.push_back(m_logicalDevice->createFramebufferUnique(framebufferCreateInfo));
	}

	vk::CommandBufferAllocateInfo commandBufferAllocInfo(
		m_graphcisPresentQueueCommandPool.get(), vk::CommandBufferLevel::ePrimary, gsl::narrow<uint32_t>(m_swapChainFramebuffers.size()));

	
	m_commandBuffers = m_logicalDevice->allocateCommandBuffersUnique(commandBufferAllocInfo);
	for (size_t i = 0, count = m_commandBuffers.size(); i < count; ++i)
	{
		vk::CommandBufferBeginInfo commandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr);
		m_commandBuffers[i]->begin(commandBufferBeginInfo);

		vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }));
		vk::RenderPassBeginInfo renderpassBeginInfo = vk::RenderPassBeginInfo{}
			.setRenderPass(m_renderpass.get())
			.setFramebuffer(m_swapChainFramebuffers[i].get())
			.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent))
			.setClearValueCount(1).setPClearValues(&clearColor);

		m_commandBuffers[i]->beginRenderPass(renderpassBeginInfo, vk::SubpassContents::eInline);

		m_commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.get());

		vk::DeviceSize offset = 0;
		m_commandBuffers[i]->bindVertexBuffers(0,1, &(m_vertexBuffer.buffer.get()), &offset),
		m_commandBuffers[i]->draw(gsl::narrow< uint32_t>(std::size(vertices)), 1, 0, 0);
		m_commandBuffers[i]->endRenderPass();
		m_commandBuffers[i]->end();
	}
}

void Application::ReCreateSwapChain()
{
	// wait until all operations completeted, so we don't cause any races
	m_logicalDevice->waitIdle();

	m_swapChainFramebuffers.clear();
	m_commandBuffers.clear();
	m_graphicsPipeline.reset();
	m_renderpass.reset();
	m_swapchainImageViews.clear();
	m_swapChain.reset();

	CreateSwapchain();
}

bool Application::Update()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (SDL_WINDOWEVENT == event.type && SDL_WINDOWEVENT_CLOSE == event.window.event)
		{
			m_logicalDevice->waitIdle();
			return false;
		}
		//handle_event(event);
	}

	constexpr uint64_t noTimeout = std::numeric_limits<uint64_t>::max();

	m_logicalDevice->waitForFences(1, &(m_inFlightFences[m_currentFrame].get()), true, noTimeout);
	m_logicalDevice->resetFences(1, &(m_inFlightFences[m_currentFrame].get()));

	uint32_t imageIndex = m_logicalDevice->acquireNextImageKHR(
		m_swapChain.get(), noTimeout, m_imageAvailableSemaphores[m_currentFrame].get(), vk::Fence()).value;

	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::SubmitInfo submitInfo = vk::SubmitInfo{}
		.setWaitSemaphoreCount(1).setPWaitSemaphores(&(m_imageAvailableSemaphores[m_currentFrame].get())).setPWaitDstStageMask(waitStages)
		.setCommandBufferCount(1).setPCommandBuffers(&(m_commandBuffers[imageIndex].get()))
		.setSignalSemaphoreCount(1).setPSignalSemaphores(&(m_renderFinishedSemaphores[m_currentFrame].get()));

	m_graphicsPresentQueue.submit(submitInfo, m_inFlightFences[m_currentFrame].get());

	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR{}
		.setWaitSemaphoreCount(1).setPWaitSemaphores(&(m_renderFinishedSemaphores[m_currentFrame].get()))
		.setSwapchainCount(1).setPSwapchains(&(m_swapChain.get())).setPImageIndices(&imageIndex);

	m_graphicsPresentQueue.presentKHR(presentInfo);


	SDL_UpdateWindowSurface(m_sdlWindow.get());

	m_currentFrame = (m_currentFrame + 1) % maxFramesInFlight;
	return true;
}
