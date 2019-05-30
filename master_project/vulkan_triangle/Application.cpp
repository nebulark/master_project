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

	template<typename T>
	vk::IndexType GetIndexBufferType()
	{
		if constexpr (std::is_same_v<uint16_t, T>) {
			return vk::IndexType::eUint16;
		}
		else if constexpr (std::is_same_v<uint32_t, T>) {
			return vk::IndexType::eUint32;
		}
		else
		{
			static_assert(false, "Type not supported");
			return vk::IndexType::eUint16;
		}
	}

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		// maybe move this out, 
		static vk::VertexInputBindingDescription GetBindingDescription()
		{
			vk::VertexInputBindingDescription bindingDescription = vk::VertexInputBindingDescription{}
				.setBinding(0) // all data is interleave in one array
				.setStride(sizeof(Vertex))
				.setInputRate(vk::VertexInputRate::eVertex);

			return bindingDescription;
		}

		static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions() {
			std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0; // needs to match shader
			attributeDescriptions[0].format = GlmTypeToVkFormat<decltype(Vertex::pos)>();
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1; // needs to match shader
			attributeDescriptions[1].format = GlmTypeToVkFormat<decltype(Vertex::color)>();
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2; // needs to match shader
			attributeDescriptions[2].format = GlmTypeToVkFormat<decltype(Vertex::texCoord)>();
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
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
		unsigned int sdl_extension_count = 0;
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
		void* /*pUserData*/)
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

	vk::UniqueImageView CreateSimpleImageView2D(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags,
		vk::Device logicalDevice)
	{
		const vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo()
			.setImage(image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(format)
			.setComponents(vk::ComponentMapping()
				.setR(vk::ComponentSwizzle::eIdentity)
				.setG(vk::ComponentSwizzle::eIdentity)
				.setB(vk::ComponentSwizzle::eIdentity)
				.setA(vk::ComponentSwizzle::eIdentity))
			.setSubresourceRange(vk::ImageSubresourceRange()
				.setAspectMask(aspectFlags)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1))
			;

		return logicalDevice.createImageViewUnique(imageViewCreateInfo);
	}

	vk::UniqueShaderModule CreateShaderModule(gsl::span<const char> shaderCode, vk::Device logicalDevice)
	{
		assert(reinterpret_cast<std::uintptr_t>(shaderCode.data()) % alignof(uint32_t) == 0);

		vk::ShaderModuleCreateInfo createInfo(vk::ShaderModuleCreateFlags{},
			static_cast<size_t>(shaderCode.size()), reinterpret_cast<const uint32_t*>(std::data(shaderCode)));

		return logicalDevice.createShaderModuleUnique(createInfo);
	}

	vk::UniqueHandle<vk::RenderPass, vk::DispatchLoaderStatic> CreateRenderpass(vk::Device logicalDevice, vk::Format swapchainFormat, vk::Format depthFormat)
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

	vk::UniqueHandle<vk::PipelineLayout, vk::DispatchLoaderStatic> CreatePipelineLayout(
		vk::Device logicalDevice, vk::DescriptorSetLayout descriptorSetLayout)
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutcreateInfo = vk::PipelineLayoutCreateInfo(
			vk::PipelineLayoutCreateFlags(),
			1, &descriptorSetLayout,
			0, nullptr);


		return logicalDevice.createPipelineLayoutUnique(pipelineLayoutcreateInfo);
	}

	vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderStatic> CreateGraphicsPipeline(
		vk::Device logicalDevice, vk::Extent2D swapchainExtent, vk::RenderPass renderpass,
		vk::PipelineLayout pipelineLayout, gsl::span<const vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreationInfos)
	{
		// fill out when we use vertex data

		std::array<vk::VertexInputAttributeDescription, 3> vertexAttributeDescription = Vertex::GetAttributeDescriptions();
		vk::VertexInputBindingDescription vertexBindingDescription = Vertex::GetBindingDescription();

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{}
			.setVertexBindingDescriptionCount(1)
			.setPVertexBindingDescriptions(&vertexBindingDescription)
			.setVertexAttributeDescriptionCount(gsl::narrow<uint32_t>(std::size(vertexAttributeDescription)))
			.setPVertexAttributeDescriptions(std::data(vertexAttributeDescription))
			;

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
			.setFrontFace(vk::FrontFace::eCounterClockwise)
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

		vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eLess)
			;

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
			&depthStencilStateCreateInfo,
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

	SimpleImage CreateImage2D(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice,
		vk::Extent2D size, vk::ImageUsageFlags usage, vk::Format format, vk::ImageTiling tiling,
		vk::MemoryPropertyFlags memoryProperties, gsl::span<const uint32_t> queueFamilyIndices = {})
	{
		vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D)
			.setExtent(vk::Extent3D(size.width, size.height, 1))
			.setMipLevels(1)
			.setArrayLayers(1)
			.setFormat(format)
			.setTiling(tiling)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setUsage(usage)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setSharingMode(vk::SharingMode::eExclusive);

		// needs to be declared outside as the reference needs to be valid until creation
		SmallArraySet<uint32_t> uniqueQueueFamilies;
		if (std::size(queueFamilyIndices) > 1)
		{
			uniqueQueueFamilies.add_unique(std::begin(queueFamilyIndices), std::end(queueFamilyIndices));
			if (uniqueQueueFamilies.size() > 1)
			{
				imageCreateInfo.setSharingMode(vk::SharingMode::eConcurrent)
					.setQueueFamilyIndexCount(gsl::narrow<uint32_t>(std::size(uniqueQueueFamilies)))
					.setPQueueFamilyIndices(std::data(uniqueQueueFamilies));
			}
		}

		SimpleImage result;
		result.image = logicalDevice.createImageUnique(imageCreateInfo);

		vk::MemoryRequirements imageMemRequirements = logicalDevice.getImageMemoryRequirements(result.image.get());
		vk::MemoryAllocateInfo allocInfo(
			imageMemRequirements.size,
			findMemoryTypeIdx(physicalDevice, imageMemRequirements.memoryTypeBits, memoryProperties)
		);

		result.imageMemory = logicalDevice.allocateMemoryUnique(allocInfo);

		logicalDevice.bindImageMemory(result.image.get(), result.imageMemory.get(), 0);

		return result;
	}


	vk::UniqueCommandBuffer allocSingleCommandBuffer(vk::Device logicaldevice, vk::CommandPool commandPool)
	{
		vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo{}
			.setCommandBufferCount(1)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandPool(commandPool);

		vk::CommandBuffer cb_temp;
		logicaldevice.allocateCommandBuffers(&allocInfo, &cb_temp);

		vk::PoolFree<vk::Device, vk::CommandPool, vk::DispatchLoaderStatic> deleter(
			logicaldevice, allocInfo.commandPool, vk::DispatchLoaderStatic());

		return vk::UniqueCommandBuffer(cb_temp, deleter);

	}

	void copyBuffer(vk::Device logicaldevice, vk::CommandPool copyCommandPool, vk::Queue transferQueue,
		vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
	{

		vk::UniqueCommandBuffer copyCommandBuffer = allocSingleCommandBuffer(logicaldevice, copyCommandPool);

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

	const std::array<Vertex, 3> vertices_triangle = {
		Vertex{{0.0f, -0.5f, 0.f}, {1.0f, 1.0f, 1.0f}},
		Vertex{{0.5f, 0.5f, 0.f}, {0.0f, 1.0f, 0.0f}},
		Vertex{{-0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}}
	};

	const std::array<Vertex, 8> vertices_quad = {
Vertex {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
Vertex	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
Vertex	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
Vertex	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

Vertex	{ {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
Vertex	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
Vertex	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
Vertex	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	const std::array<uint16_t, 12> indices_quad = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};

	constexpr bool drawQuad = true;

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};
}

Application::Application()
{
	constexpr int width = 1920 / 2;
	constexpr int height = 1080 / 2;

	m_sdlWindow = WindowPtr{
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
		"VK_LAYER_KHRONOS_validation",
		"VK_LAYER_LUNARG_standard_validation"
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
	deviceRequirements.requiredFeatures = vk::PhysicalDeviceFeatures()
		.setSamplerAnisotropy(true);

	std::optional<VulkanDevice::PickDeviceResult> maybeDeviceResult =
		VulkanDevice::PickPhysicalDevice(m_vkInstance.get(), deviceRequirements, &(m_surface.get()));

	assert(maybeDeviceResult);
	m_physicalDevice = maybeDeviceResult->device;
	m_deviceQueueInfo = std::move(maybeDeviceResult->queueResult);


	m_logicalDevice = VulkanDevice::CreateLogicalDevice(
		m_physicalDevice, m_deviceQueueInfo, enabledValidationLayers, deviceExtensions, deviceRequirements.requiredFeatures);

	m_graphicsPresentQueue = m_logicalDevice->getQueue(m_deviceQueueInfo[0].familyIndex, m_deviceQueueInfo[0].offset);
	m_transferQueue = m_logicalDevice->getQueue(m_deviceQueueInfo[1].familyIndex, m_deviceQueueInfo[1].offset);

	std::vector<char> vertShaderCode = FileHelper::LoadFileContent("triangle.vert.spv");
	std::vector<char> fragShaderCode = FileHelper::LoadFileContent("triangle.frag.spv");

	m_vertShaderModule = CreateShaderModule(vertShaderCode, m_logicalDevice.get());
	m_fragShaderModule = CreateShaderModule(fragShaderCode, m_logicalDevice.get());

	vk::DescriptorSetLayoutBinding uboLayoutBinding = vk::DescriptorSetLayoutBinding{}
		.setBinding(0) // matches Shader code
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		;

	vk::DescriptorSetLayoutBinding samplerLayoutBinding = vk::DescriptorSetLayoutBinding{}
		.setBinding(1) // matches Shader code
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		;

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo{}
		.setBindingCount(static_cast<uint32_t>(bindings.size()))
		.setPBindings(bindings.data());

	m_descriptorSetLayout = m_logicalDevice->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

	m_pipelineLayout = CreatePipelineLayout(m_logicalDevice.get(), m_descriptorSetLayout.get());

	const uint32_t familyIndices_present_and_transfer[] = { m_deviceQueueInfo[0].familyIndex, m_deviceQueueInfo[1].familyIndex };


	// command buffers

	vk::CommandPoolCreateInfo commandPoolCreateInfo_graphcisPresentQueue(
		vk::CommandPoolCreateFlags() /*there are possible flags*/,
		m_deviceQueueInfo[0].familyIndex);
	m_graphcisPresentQueueCommandPool = m_logicalDevice->createCommandPoolUnique(commandPoolCreateInfo_graphcisPresentQueue);

	vk::CommandPoolCreateInfo commandPoolCreateInfo_transferQueue(
		vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		m_deviceQueueInfo[1].familyIndex);
	m_transferQueueCommandPool = m_logicalDevice->createCommandPoolUnique(commandPoolCreateInfo_transferQueue);

	{
		int texWidth, texheight, texChannels;
		stbi_uc* pixels = stbi_load("texture.jpg", &texWidth, &texheight, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texWidth) * texheight * 4;
		assert(pixels);

		SimpleBuffer imageStagingBuffer = createBuffer(m_logicalDevice.get(), m_physicalDevice, gsl::narrow<uint32_t>(imageSize), vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);
		{
			void* bufferPtr = m_logicalDevice->mapMemory(imageStagingBuffer.bufferMemory.get(), 0, imageSize);
			std::memcpy(bufferPtr, pixels, static_cast<size_t>(imageSize));
			m_logicalDevice->unmapMemory(imageStagingBuffer.bufferMemory.get());
		}

		stbi_image_free(pixels);

		m_textureImage = CreateImage2D(m_logicalDevice.get(), m_physicalDevice, vk::Extent2D(texWidth, texheight),
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::Format::eR8G8B8A8Unorm,
			vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal, familyIndices_present_and_transfer);

		vk::UniqueCommandBuffer copyCommandBuffer = allocSingleCommandBuffer(m_logicalDevice.get(), m_transferQueueCommandPool.get());

		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		copyCommandBuffer->begin(beginInfo);

		//---------------
		{
			vk::ImageMemoryBarrier imageMemoryBarier = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(m_textureImage.image.get())
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseMipLevel(0)
					.setLevelCount(1)
					.setBaseArrayLayer(0)
					.setLayerCount(1))
				.setSrcAccessMask(vk::AccessFlags())
				.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

			copyCommandBuffer->pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(), {}, {}, imageMemoryBarier);
		}

		//-----------------------
		vk::BufferImageCopy bufferImageCopy = vk::BufferImageCopy()
			.setBufferOffset(0)
			.setBufferRowLength(0)
			.setBufferImageHeight(0)
			.setImageSubresource(vk::ImageSubresourceLayers()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(0)
				.setBaseArrayLayer(0)
				.setLayerCount(1))
			.setImageOffset(vk::Offset3D(0, 0, 0))
			.setImageExtent(vk::Extent3D(texWidth, texheight, 1));

		copyCommandBuffer->copyBufferToImage(imageStagingBuffer.buffer.get(), m_textureImage.image.get(),
			vk::ImageLayout::eTransferDstOptimal, bufferImageCopy);

		//-------------------------------------
		{
			vk::ImageMemoryBarrier imageMemoryBarier = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(m_textureImage.image.get())
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseMipLevel(0)
					.setLevelCount(1)
					.setBaseArrayLayer(0)
					.setLayerCount(1))
				.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
				.setDstAccessMask(vk::AccessFlags());

			copyCommandBuffer->pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::DependencyFlags(), {}, {}, imageMemoryBarier);
		}

		copyCommandBuffer->end();
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&(copyCommandBuffer.get()))
			;

		m_transferQueue.submit(submitInfo, vk::Fence());
		m_transferQueue.waitIdle();

		vk::ImageViewCreateInfo viewInfo = vk::ImageViewCreateInfo()
			.setImage(m_textureImage.image.get())
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eR8G8B8A8Unorm)
			.setSubresourceRange(vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1))
			;

		m_textureImageView = m_logicalDevice->createImageViewUnique(viewInfo);
		vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat)
			.setAnisotropyEnable(true)
			.setMaxAnisotropy(16)
			.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
			.setUnnormalizedCoordinates(false)
			.setCompareEnable(false)
			.setCompareOp(vk::CompareOp::eAlways)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMipLodBias(0.f)
			.setMinLod(0.f)
			.setMaxLod(0.f)
			;

		m_textureSampler = m_logicalDevice->createSamplerUnique(samplerCreateInfo);
	}


	for (size_t i = 0; i < maxFramesInFlight; ++i)
	{
		m_imageAvailableSemaphores[i] = m_logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		m_renderFinishedSemaphores[i] = m_logicalDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		m_inFlightFences[i] = m_logicalDevice->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}

	m_currentFrame = 0;




	// Quad
	if (drawQuad)
	{
		const vk::DeviceSize indexBufferSize = sizeof(indices_quad[0]) * std::size(indices_quad);
		const vk::DeviceSize vertexBufferSize = sizeof(vertices_quad[0]) * std::size(vertices_quad);
		const uint32_t stageBufferIdxBegin = 0;
		const uint32_t stageBufferIdxEnd = stageBufferIdxBegin + indexBufferSize;

		const uint32_t stageBufferVertexBegin = VulkanUtils::AlignUp<uint32_t>(indexBufferSize, alignof(Vertex));
		const uint32_t stageBufferVertexEnd = stageBufferVertexBegin + vertexBufferSize;

		const vk::DeviceSize stageBufferSize = stageBufferVertexEnd;


		SimpleBuffer stagingBuffer = createBuffer(
			m_logicalDevice.get(), m_physicalDevice, gsl::narrow<uint32_t>(stageBufferSize),
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		{
			std::byte* mappedVertexBufferMemory = reinterpret_cast<std::byte*>(
				m_logicalDevice->mapMemory(stagingBuffer.bufferMemory.get(), 0, stagingBuffer.size));

			std::memcpy(mappedVertexBufferMemory + stageBufferIdxBegin, std::data(indices_quad), indexBufferSize);
			std::memcpy(mappedVertexBufferMemory + stageBufferVertexBegin, std::data(vertices_quad), vertexBufferSize);
			m_logicalDevice->unmapMemory(stagingBuffer.bufferMemory.get());
		}

		m_indexBuffer_quad = createBuffer(m_logicalDevice.get(), m_physicalDevice,
			indexBufferSize,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			familyIndices_present_and_transfer);

		m_vertexBuffer_quad = createBuffer(m_logicalDevice.get(), m_physicalDevice,
			vertexBufferSize,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			familyIndices_present_and_transfer);

		vk::UniqueCommandBuffer copyCommandBuffer = allocSingleCommandBuffer(m_logicalDevice.get(), m_transferQueueCommandPool.get());

		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		copyCommandBuffer->begin(beginInfo);
		{
			vk::BufferCopy bufferCopy_stage_to_index(stageBufferIdxBegin, 0, indexBufferSize);
			copyCommandBuffer->copyBuffer(stagingBuffer.buffer.get(), m_indexBuffer_quad.buffer.get(), bufferCopy_stage_to_index);
		}
		{
			vk::BufferCopy bufferCopy_stage_to_vertex(stageBufferVertexBegin, 0, vertexBufferSize);
			copyCommandBuffer->copyBuffer(stagingBuffer.buffer.get(), m_vertexBuffer_quad.buffer.get(), bufferCopy_stage_to_vertex);
		}
		copyCommandBuffer->end();

		vk::SubmitInfo submitInfo = vk::SubmitInfo{}
			.setCommandBufferCount(1)
			.setPCommandBuffers(&(copyCommandBuffer.get()));

		m_transferQueue.submit(submitInfo, vk::Fence{});
		m_transferQueue.waitIdle();

	}
	// Triangel !drawQuad
	else
	{
		const vk::DeviceSize verticesByteSize = sizeof(vertices_triangle[0]) * std::size(vertices_triangle);
		m_vertexBuffer = createBuffer(
			m_logicalDevice.get(), m_physicalDevice,
			verticesByteSize,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			familyIndices_present_and_transfer);

		SimpleBuffer stagingBuffer = createBuffer(
			m_logicalDevice.get(), m_physicalDevice, verticesByteSize, vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		{
			void* mappedVertexBufferMemory = m_logicalDevice->mapMemory(stagingBuffer.bufferMemory.get(), 0, stagingBuffer.size);
			std::memcpy(mappedVertexBufferMemory, std::data(vertices_triangle), stagingBuffer.size);
			m_logicalDevice->unmapMemory(stagingBuffer.bufferMemory.get());
		}

		copyBuffer(m_logicalDevice.get(), m_transferQueueCommandPool.get(), m_transferQueue,
			stagingBuffer.buffer.get(), m_vertexBuffer.buffer.get(), verticesByteSize);
	}

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

	assert(currentWidth >= 0 && currentHeight >= 0);

	const vk::SurfaceCapabilitiesKHR capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());
	m_swapchainExtent = VulkanUtils::ChooseExtent(
		capabilities,
		vk::Extent2D(
			static_cast<uint32_t>(currentWidth),
			static_cast<uint32_t>(currentHeight))
	);

	const uint32_t imageCount = VulkanUtils::ChooseImageCount(capabilities, capabilities.minImageCount + 1);

	const vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
		.setSurface(m_surface.get())
		.setMinImageCount(imageCount)
		.setImageFormat(swapchainFormat.format)
		.setImageColorSpace(swapchainFormat.colorSpace)
		.setImageExtent(m_swapchainExtent)
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

	vk::DeviceSize uniformBufferSize = sizeof(UniformBufferObject);
	for (size_t i = 0; i < swapchainImages.size(); ++i)
	{
		m_uniformBuffers.push_back(
			createBuffer(
				m_logicalDevice.get(), m_physicalDevice, gsl::narrow<uint32_t>(uniformBufferSize), vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
			));
	}
	
	// depth Buffer
		vk::Format preferedDepthFormats[] = { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
		vk::Format depthFormat = VulkanUtils::ChooseFormat(m_physicalDevice, preferedDepthFormats, vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	{
		m_depthBuffer = CreateImage2D(m_logicalDevice.get(), m_physicalDevice, m_swapchainExtent,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			depthFormat, vk::ImageTiling::eOptimal, vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_depthBufferView = CreateSimpleImageView2D(m_depthBuffer.image.get(), depthFormat, vk::ImageAspectFlagBits::eDepth,
			m_logicalDevice.get());
		vk::UniqueCommandBuffer transitionLayoutCommandBuffer = allocSingleCommandBuffer(m_logicalDevice.get(),m_graphcisPresentQueueCommandPool.get());

		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		transitionLayoutCommandBuffer->begin(beginInfo);

		//---------------
		{
			vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eDepth;
			if (VulkanUtils::HasStencilComponent(depthFormat))
			{
				aspectFlags |= vk::ImageAspectFlagBits::eStencil;
			}

			vk::ImageMemoryBarrier imageMemoryBarier = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(m_textureImage.image.get())
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(aspectFlags)
					.setBaseMipLevel(0)
					.setLevelCount(1)
					.setBaseArrayLayer(0)
					.setLayerCount(1))
				.setSrcAccessMask(vk::AccessFlags())
				.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

			transitionLayoutCommandBuffer->pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests,
				vk::DependencyFlags(), {}, {}, imageMemoryBarier);
			transitionLayoutCommandBuffer->end();

			vk::SubmitInfo submitInfo = vk::SubmitInfo()
				.setCommandBufferCount(1).setPCommandBuffers(&(transitionLayoutCommandBuffer.get()))
				;

			m_graphicsPresentQueue.submit(submitInfo,vk::Fence());
			m_graphicsPresentQueue.waitIdle();
		}
	}
	vk::DescriptorPoolSize descriptorPoolSize_ubo = vk::DescriptorPoolSize()
		.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(gsl::narrow<uint32_t>(swapchainImages.size()));


	vk::DescriptorPoolSize descriptorPoolSize_sampler = vk::DescriptorPoolSize()
		.setType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(gsl::narrow<uint32_t>(swapchainImages.size()));

	std::array<vk::DescriptorPoolSize, 2> descriptorPoolSizes = { descriptorPoolSize_ubo, descriptorPoolSize_sampler };
	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo = vk::DescriptorPoolCreateInfo()
		.setPoolSizeCount(static_cast<uint32_t>(descriptorPoolSizes.size())).setPPoolSizes(descriptorPoolSizes.data())
		.setMaxSets(gsl::narrow<uint32_t>(swapchainImages.size()));

	m_descriptorPool = m_logicalDevice->createDescriptorPoolUnique(descriptorPoolCreateInfo);

	std::vector<vk::DescriptorSetLayout> layouts(swapchainImages.size(), m_descriptorSetLayout.get());
	vk::DescriptorSetAllocateInfo descriptorSetAllocInfo = vk::DescriptorSetAllocateInfo{}
		.setDescriptorPool(m_descriptorPool.get())
		.setDescriptorSetCount(gsl::narrow<uint32_t>(swapchainImages.size()))
		.setPSetLayouts(layouts.data());

	m_descriptorSets = m_logicalDevice->allocateDescriptorSets(descriptorSetAllocInfo);

	for (size_t i = 0; i < m_descriptorSets.size(); i++)
	{
		vk::DescriptorBufferInfo bufferInfo = vk::DescriptorBufferInfo{}
			.setBuffer(m_uniformBuffers[i].buffer.get())
			.setOffset(0)
			.setRange(sizeof(UniformBufferObject));

		vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo{}
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(m_textureImageView.get())
			.setSampler(m_textureSampler.get());
		std::array<vk::WriteDescriptorSet, 2> writeDescriptorSets;

		writeDescriptorSets[0] = vk::WriteDescriptorSet{}
			.setDstSet(m_descriptorSets[i])
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1).setPBufferInfo(&bufferInfo);

		writeDescriptorSets[1] = vk::WriteDescriptorSet{}
			.setDstSet(m_descriptorSets[i])
			.setDstBinding(1)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(1).setPImageInfo(&imageInfo);

		m_logicalDevice->updateDescriptorSets(writeDescriptorSets, {});
	}


	m_renderpass = CreateRenderpass(m_logicalDevice.get(), swapchainFormat.format, depthFormat);

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
		m_logicalDevice.get(), m_swapchainExtent, m_renderpass.get(), m_pipelineLayout.get(), pipelineShaderStageCreationInfos);


	m_swapchainImageViews.reserve(swapchainImages.size());

	for (const vk::Image& image : swapchainImages)
	{
		m_swapchainImageViews.push_back(CreateSimpleImageView2D(image, swapchainFormat.format, vk::ImageAspectFlagBits::eColor,
			m_logicalDevice.get()));
	}

	m_swapChainFramebuffers.reserve(m_swapchainImageViews.size());
	for (vk::UniqueImageView& imageView : m_swapchainImageViews)
	{
		vk::ImageView attachments[] = { imageView.get(), m_depthBufferView.get() };
		vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo{}
			.setRenderPass(m_renderpass.get())
			.setAttachmentCount(static_cast<uint32_t>(std::size(attachments))).setPAttachments(attachments)
			.setWidth(m_swapchainExtent.width)
			.setHeight(m_swapchainExtent.height)
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

		vk::ClearValue clearValues[] = {
			vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }),
			vk::ClearDepthStencilValue(1.f, 0)
		};
		vk::RenderPassBeginInfo renderpassBeginInfo = vk::RenderPassBeginInfo{}
			.setRenderPass(m_renderpass.get())
			.setFramebuffer(m_swapChainFramebuffers[i].get())
			.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), m_swapchainExtent))
			.setClearValueCount(static_cast<uint32_t>(std::size(clearValues))).setPClearValues(clearValues);

		m_commandBuffers[i]->beginRenderPass(renderpassBeginInfo, vk::SubpassContents::eInline);

		m_commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.get());

		vk::DeviceSize offset = 0;
		constexpr int firstBinding = 0;

		if (drawQuad)
		{
			m_commandBuffers[i]->bindIndexBuffer(
				m_indexBuffer_quad.buffer.get(), offset, GetIndexBufferType<decltype(indices_quad)::value_type>());

			m_commandBuffers[i]->bindVertexBuffers(firstBinding, m_vertexBuffer_quad.buffer.get(), offset);
			m_commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0,
				m_descriptorSets[i], {});

			m_commandBuffers[i]->drawIndexed(
				gsl::narrow<uint32_t>(
					indices_quad.size()
					), 1, 0, 0, 0);
		}
		else
		{
			m_commandBuffers[i]->bindVertexBuffers(firstBinding, m_vertexBuffer.buffer.get(), offset);
			m_commandBuffers[i]->draw(3, 1, 0, 0);
		}

		m_commandBuffers[i]->endRenderPass();
		m_commandBuffers[i]->end();
	}

	m_lastTime = std::chrono::steady_clock::now();
	m_rotationDeg = 0.f;
}

void Application::ReCreateSwapChain()
{
	// wait until all operations completeted, so we don't cause any races
	m_logicalDevice->waitIdle();

	m_swapChainFramebuffers.clear();
	m_commandBuffers.clear();
	m_graphicsPipeline.reset();
	m_renderpass.reset();
	m_descriptorSets.clear();
	m_descriptorPool.reset();
	m_swapchainImageViews.clear();
	m_uniformBuffers.clear();
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

	m_logicalDevice->waitForFences(m_inFlightFences[m_currentFrame].get(), true, noTimeout);
	m_logicalDevice->resetFences(m_inFlightFences[m_currentFrame].get());

	uint32_t imageIndex = m_logicalDevice->acquireNextImageKHR(
		m_swapChain.get(), noTimeout, m_imageAvailableSemaphores[m_currentFrame].get(), vk::Fence()).value;


	auto currentTime = std::chrono::high_resolution_clock::now();
	const float deltaSeconds = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - m_lastTime).count();
	m_lastTime = currentTime;

	m_rotationDeg = std::fmod(m_rotationDeg + deltaSeconds * 90.f, 360.f);

	UniformBufferObject ubo;
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotationDeg), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f),
		static_cast<float>(m_swapchainExtent.width) / static_cast<float>(m_swapchainExtent.height), 0.1f, 10.0f);

	// flip Y coordinate
	ubo.proj[1][1] *= -1;
	{
		void* uboGpuMem = m_logicalDevice->mapMemory(m_uniformBuffers[imageIndex].bufferMemory.get(), 0, sizeof(ubo));
		std::memcpy(uboGpuMem, &ubo, sizeof(ubo));
		m_logicalDevice->unmapMemory(m_uniformBuffers[imageIndex].bufferMemory.get());
	}

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
