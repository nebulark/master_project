#include "pch.hpp"
#include "GraphicBackend.hpp"
#include "GetSizeUint32.hpp"
#include "common/VulkanDevice.hpp"
#include "common/VulkanDebug.hpp"
#include "common/FileHelper.hpp"
#include "common/VulkanUtils.hpp"
#include "Renderpass.hpp"
#include "PushConstants.hpp"
#include "GraphicsPipeline.hpp"
#include "DebugUtils.hpp"
#include "CommandBufferUtils.hpp"
#include "Vertex.hpp"
#include "Camera.hpp"
#include "UniformBufferObjects.hpp"

namespace
{

	vk::UniqueInstance CreateVulkanInstance(
		const char* name,
		gsl::span<const char*> enabledLayers,
		gsl::span<const char*> enabledExtensions)
	{
		vk::ApplicationInfo application_info(name, 1, "My Engine", 1, VK_API_VERSION_1_1);


		vk::InstanceCreateInfo instanceCreateInfo({}, &application_info,
			GetSizeUint32(enabledLayers), std::data(enabledLayers),
			GetSizeUint32(enabledExtensions), std::data(enabledExtensions));

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


}


void GraphicsBackend::Init(SDL_Window* window)
{
	const char* enabledValidationLayers[] =
	{
		// Enable standard validation layer to find as much errors as possible!
		"VK_LAYER_KHRONOS_validation",
		"VK_LAYER_LUNARG_standard_validation"
	};

	std::vector<const char*> enabledExtensions = GetSdlExtensions(window);
	enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);


	assert(VulkanUtils::SupportsValidationLayers(enabledValidationLayers));

	m_vkInstance = CreateVulkanInstance("Vulkan Rasterizer", enabledValidationLayers, enabledExtensions);
	VulkanDebug::LoadDebugUtilsExtension(m_vkInstance.get());

	m_debugUtilsMessenger = DebugUtils::CreateDebugUtilsMessenger(m_vkInstance.get());
	m_surface = CreateSurface(m_vkInstance.get(), window);

	const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VulkanDevice::QueueRequirement queueRequirement[1];
	queueRequirement[0].canPresent = true;
	queueRequirement[0].mincount = 1;
	queueRequirement[0].minFlags = vk::QueueFlagBits::eGraphics;

	VulkanDevice::DeviceRequirements deviceRequirements;
	deviceRequirements.queueRequirements = queueRequirement;
	deviceRequirements.requiredExtensions = deviceExtensions;
	deviceRequirements.requiredFeatures = vk::PhysicalDeviceFeatures()
		.setSamplerAnisotropy(true);

	std::optional<VulkanDevice::PickDeviceResult> maybeDeviceResult =
		VulkanDevice::PickPhysicalDevice(m_vkInstance.get(), deviceRequirements, &(m_surface.get()));

	assert(maybeDeviceResult);
	m_physicalDevice = maybeDeviceResult->device;
	m_device = VulkanDevice::CreateLogicalDevice(
		m_physicalDevice, maybeDeviceResult->queueResult, enabledValidationLayers, deviceExtensions, deviceRequirements.requiredFeatures);


	VmaAllocatorCreateInfo vmaAllocCreateInfo = {};
	vmaAllocCreateInfo.device = m_device.get();
	vmaAllocCreateInfo.physicalDevice = m_physicalDevice;
	m_allocator = VmaRAII::CreateVmaAllocatorUnique(vmaAllocCreateInfo);
	m_BufferPool.Init(m_allocator.get());
	m_ImagePool.Init(m_allocator.get());

	m_graphicsPresentQueueInfo = maybeDeviceResult->queueResult[0];
	{
		m_graphicsPresentQueues = m_device->getQueue(m_graphicsPresentQueueInfo.familyIndex, m_graphicsPresentQueueInfo.offset);
		const vk::CommandPoolCreateInfo graphcisPresentCommandPoolInfo = vk::CommandPoolCreateInfo{}
			.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
			.setQueueFamilyIndex(m_graphicsPresentQueueInfo.familyIndex);

		for (size_t i = 0; i < std::size(m_graphicsPresentCommandPools); ++i)
		{
			m_graphicsPresentCommandPools[i] = m_device->createCommandPoolUnique(graphcisPresentCommandPoolInfo);
			m_graphicsPresentBuffer[i] = CbUtils::AllocateSingle(m_device.get(), m_graphicsPresentCommandPools[i].get());
		}
	}


	// Load Textures 
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load("chalet.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		assert(pixels);
		vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texWidth) * texHeight * 4;


		VkBufferCreateInfo stagingBufferInfo = vk::BufferCreateInfo{}
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
			.setSize(imageSize)
			.setSharingMode(vk::SharingMode::eExclusive);

		VmaAllocationCreateInfo vmaAllocInfo = {};
		vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
		VmaAllocation stageBufferAllocation;
		VkBuffer stageBuffer;

		VkResult cresult = vmaCreateBuffer(m_allocator.get(), &stagingBufferInfo, &vmaAllocInfo, &stageBuffer, &stageBufferAllocation, nullptr);
		vk::Result result = static_cast<vk::Result>(cresult);
		assert(result == vk::Result::eSuccess);
		{
			void* bufferPtr = nullptr;
			VkResult result = vmaMapMemory(m_allocator.get(), stageBufferAllocation, &bufferPtr);
			assert(result == VkResult::VK_SUCCESS);

			std::memcpy(bufferPtr, pixels, static_cast<size_t>(imageSize));
			vmaUnmapMemory(m_allocator.get(), stageBufferAllocation);
		}

		stbi_image_free(pixels);


		vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo{}
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
			.setFormat(vk::Format::eR8G8B8A8Unorm)
			.setExtent(vk::Extent3D(texWidth, texHeight, 1))
			.setTiling(vk::ImageTiling::eOptimal)
			.setMipLevels(1)
			.setArrayLayers(1)
			.setImageType(vk::ImageType::e2D)
			;

		VmaAllocationCreateInfo vmaAllocImage = {};
		vmaAllocImage.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		m_textureImage = m_ImagePool.Alloc(imageCreateInfo, vmaAllocImage);


		VulkanDebug::SetObjectName(m_device.get(), m_textureImage, "texture Image");

		vk::UniqueCommandBuffer loadBuffer = CbUtils::AllocateSingle(m_device.get(), m_graphicsPresentCommandPools[0].get());
		loadBuffer->begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		{
			vk::ImageMemoryBarrier imageMemoryBarier = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(m_textureImage)
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseMipLevel(0)
					.setLevelCount(1)
					.setBaseArrayLayer(0)
					.setLayerCount(1))
				.setSrcAccessMask(vk::AccessFlags())
				.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

			loadBuffer->pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, imageMemoryBarier);
		}
		{
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
				.setImageExtent(vk::Extent3D(texWidth, texHeight, 1));

			loadBuffer->copyBufferToImage(stageBuffer, m_textureImage, vk::ImageLayout::eTransferDstOptimal, bufferImageCopy);
		}
		{
			vk::ImageMemoryBarrier imageMemoryBarier1 = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(m_textureImage)
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseMipLevel(0)
					.setLevelCount(1)
					.setBaseArrayLayer(0)
					.setLayerCount(1))
				.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

			loadBuffer->pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, imageMemoryBarier1);

			}

		loadBuffer->end();

		// Todo keep buffer around and wait only when required instead of immediately
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&(loadBuffer.get()))
			;

		m_graphicsPresentQueues.submit(submitInfo, vk::Fence());
		m_graphicsPresentQueues.waitIdle();

		vmaDestroyBuffer(m_allocator.get(), stageBuffer, stageBufferAllocation);

		vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo{}
			.setImage(m_textureImage)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eR8G8B8A8Unorm)
			.setSubresourceRange(vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1));

		m_textureImageView = m_device->createImageViewUnique(imageViewCreateInfo);
	}

	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	m_swapchain = Swapchain::Create(m_physicalDevice, m_device.get(), m_surface.get(), vk::Extent2D(windowWidth, windowHeight));

	vk::Format preferedDepthFormats[] = { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
	m_depthFormat = VulkanUtils::ChooseFormat(m_physicalDevice, preferedDepthFormats, vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	{
		vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
			.setFormat(m_depthFormat)
			.setExtent(vk::Extent3D(m_swapchain.extent.width, m_swapchain.extent.height, 1))
			.setTiling(vk::ImageTiling::eOptimal)
			.setArrayLayers(1)
			.setMipLevels(1);

		VmaAllocationCreateInfo vmaAllocImage = {};
		vmaAllocImage.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		m_depthBuffer = m_ImagePool.Alloc(imageCreateInfo, vmaAllocImage);
		vk::UniqueCommandBuffer transitionLayoutCommandBuffer =
			CbUtils::AllocateSingle(m_device.get(), m_graphicsPresentCommandPools[0].get());

		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		transitionLayoutCommandBuffer->begin(beginInfo);

		//---------------
		{
			vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eDepth;
			if (VulkanUtils::HasStencilComponent(m_depthFormat))
			{
				aspectFlags |= vk::ImageAspectFlagBits::eStencil;
			}

			vk::ImageMemoryBarrier imageMemoryBarier = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(m_depthBuffer)
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

			m_graphicsPresentQueues.submit(submitInfo, vk::Fence());
			m_graphicsPresentQueues.waitIdle();

			m_depthBufferView = m_device->createImageViewUnique(vk::ImageViewCreateInfo{}
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(m_depthFormat)
				.setImage(m_depthBuffer)
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eDepth)
					.setBaseMipLevel(0)
					.setLevelCount(1)
					.setBaseArrayLayer(0)
					.setLayerCount(1)));
		}
	}
	m_colorDepthRenderPass = Renderpass::Create_withDepth(m_device.get(), m_swapchain.surfaceFormat.format, m_depthFormat);
	{

		vk::FramebufferCreateInfo framebufferPrototype = vk::FramebufferCreateInfo{}
			.setWidth(m_swapchain.extent.width)
			.setHeight(m_swapchain.extent.height)
			.setRenderPass(m_colorDepthRenderPass.get())
			.setLayers(1);
		for (const vk::UniqueImageView& imageView : m_swapchain.imageViews)
		{
			vk::ImageView fbAttachments[] = { imageView.get(), m_depthBufferView.get() };
			m_framebuffer.push_back(m_device->createFramebufferUnique(
				vk::FramebufferCreateInfo{ framebufferPrototype }
				.setAttachmentCount(GetSizeUint32(fbAttachments))
				.setPAttachments(fbAttachments))
			);
		}
	}

	m_vertShaderModule = VulkanUtils::CreateShaderModuleFromFile("shader.vert.spv", m_device.get());
	m_fragShaderModule = VulkanUtils::CreateShaderModuleFromFile("shader.frag.spv", m_device.get());



	// Descriptor Sets - Combined Image Sampler
	{
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_texture[] = {
		vk::DescriptorSetLayoutBinding{}
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment),

			};

			vk::DescriptorSetLayoutCreateInfo descritorSetLayoutInfo_texture = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_texture))
				.setPBindings(descriptorSetBinding_texture)
				;

			m_descriptorSetLayout_texture = m_device->createDescriptorSetLayoutUnique(descritorSetLayoutInfo_texture);
		}
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_ubo[] =
			{
				vk::DescriptorSetLayoutBinding{}
					.setBinding(0) // matches Shader code
					.setDescriptorType(vk::DescriptorType::eUniformBuffer)
					.setDescriptorCount(1)
					.setStageFlags(vk::ShaderStageFlagBits::eVertex),
			};

			vk::DescriptorSetLayoutCreateInfo descritorSetLayoutInfo_ubo = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_ubo))
				.setPBindings(descriptorSetBinding_ubo)
				;

			m_descriptorSetLayout_ubo = m_device->createDescriptorSetLayoutUnique(descritorSetLayoutInfo_ubo);
		}
		vk::DescriptorPoolSize descritproPoolSizes[] =
		{

		vk::DescriptorPoolSize()
			.setType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(1),

		vk::DescriptorPoolSize{}
			.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(2),



		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo_sampler = vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(GetSizeUint32(descritproPoolSizes)).setPPoolSizes(descritproPoolSizes)
			.setMaxSets(3);


		m_descriptorPool = m_device->createDescriptorPoolUnique(descriptorPoolCreateInfo_sampler);

		{
			vk::DescriptorSetLayout layouts[] = {
				m_descriptorSetLayout_texture.get(),
				m_descriptorSetLayout_ubo.get(),
				m_descriptorSetLayout_ubo.get() // we need two as we write it each frame, so we can leave one alone until its finished		
			};

			vk::DescriptorSetAllocateInfo descritproSetAllocateInfo = vk::DescriptorSetAllocateInfo{}
				.setDescriptorPool(m_descriptorPool.get())
				.setDescriptorSetCount(GetSizeUint32(layouts))
				.setPSetLayouts(layouts);


			std::vector<vk::DescriptorSet> descriptorSets = m_device->allocateDescriptorSets(descritproSetAllocateInfo);
			m_descriptorSet_texture = std::move(descriptorSets[0]);
			m_descriptorSet_ubo[0] = std::move(descriptorSets[1]);
			m_descriptorSet_ubo[1] = std::move(descriptorSets[2]);
		}
		{
			vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
				.setMagFilter(vk::Filter::eLinear)
				.setMinFilter(vk::Filter::eLinear)
				.setAddressModeU(vk::SamplerAddressMode::eRepeat)
				.setAddressModeV(vk::SamplerAddressMode::eRepeat)
				.setAddressModeW(vk::SamplerAddressMode::eRepeat)
				//	.setAnisotropyEnable(true)
				//	.setMaxAnisotropy(16)
				.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
				.setUnnormalizedCoordinates(false)
				.setCompareEnable(false)
				.setCompareOp(vk::CompareOp::eAlways)
				.setMipmapMode(vk::SamplerMipmapMode::eLinear)
				.setMipLodBias(0.f)
				.setMinLod(0.f)
				.setMaxLod(static_cast<float>(1))
				;

			m_textureSampler = m_device->createSamplerUnique(samplerCreateInfo);

			vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo{}
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(m_textureImageView.get())
				.setSampler(m_textureSampler.get());

			vk::WriteDescriptorSet writeDescriptorSet = vk::WriteDescriptorSet{}
				.setDstSet(m_descriptorSet_texture)
				.setDstBinding(0) // matches shader code
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDstArrayElement(0)
				.setDescriptorCount(1).setPImageInfo(&imageInfo);

			m_device->updateDescriptorSets(writeDescriptorSet, {});
		}
		{

			vk::DeviceSize uniformBufferSize = sizeof(Ubo_GlobalRenderData);
			for (size_t i = 0; i < m_ubo_buffer.size(); ++i)
			{
				const vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo{}
					.setSize(uniformBufferSize)
					.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
					.setSharingMode(vk::SharingMode::eExclusive);

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

				m_ubo_buffer[i] = m_BufferPool.Alloc(bufferCreateInfo, allocCreateInfo);
			}


			std::array<vk::DescriptorBufferInfo, std::tuple_size_v<decltype(m_ubo_buffer)>> descriptorBufferInfos;
			std::array<vk::WriteDescriptorSet, std::tuple_size_v<decltype(m_ubo_buffer)>> writeDescriptorSets;

			for (size_t i = 0; i < descriptorBufferInfos.size(); ++i)
			{

				descriptorBufferInfos[i] = vk::DescriptorBufferInfo{}
					.setBuffer(m_ubo_buffer[i])
					.setOffset(0)
					.setRange(sizeof(Ubo_GlobalRenderData));


				writeDescriptorSets[i] = vk::WriteDescriptorSet{}
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType::eUniformBuffer)
					.setDstArrayElement(0)
					.setDstBinding(0) // matches shader code
					.setDstSet(m_descriptorSet_ubo[i])
					.setPBufferInfo(&descriptorBufferInfos[i])
					;
			}

			m_device->updateDescriptorSets(writeDescriptorSets, {});


		}
	}

	vk::PushConstantRange pushConstantRange_modelViewProjection = vk::PushConstantRange{}
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setOffset(0)
		.setSize(sizeof(PushConstant_ModelMat));


	vk::DescriptorSetLayout layouts[] = { m_descriptorSetLayout_texture.get(), m_descriptorSetLayout_ubo.get() };
	vk::PipelineLayoutCreateInfo pipelineLayoutcreateInfo = vk::PipelineLayoutCreateInfo{}
		.setSetLayoutCount(GetSizeUint32(layouts)).setPSetLayouts(layouts)
		.setPushConstantRangeCount(1).setPPushConstantRanges(&pushConstantRange_modelViewProjection);

	m_pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutcreateInfo);
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

	m_graphicsPipeline = GraphicsPipeline::CreateGraphicsPipeline_static_simple(
		m_device.get(),
		m_swapchain.extent,
		m_colorDepthRenderPass.get(),
		m_pipelineLayout.get(),
		pipelineShaderStageCreationInfos);
	for (int i = 0; i < MaxInFlightFrames; ++i)
	{

		m_frameFence[i] = m_device->createFenceUnique(vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled));
		m_imageAvailableSem[i] = m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
		m_renderFinishedSem[i] = m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
	}

	m_staticSceneData = std::make_unique<StaticSceneData>(m_BufferPool);

	const char* objsToLoad[] = { "torus.obj" };
	// use graphics present queue to avoid ownership transfer
	m_staticSceneData->LoadObjs(objsToLoad, m_device.get(), m_graphicsPresentCommandPools[0].get(), m_graphicsPresentQueues);

}

void GraphicsBackend::Render(const Camera& camera)
{
	constexpr uint64_t noTimeout = std::numeric_limits<uint64_t>::max();
	m_device->waitForFences(m_frameFence[m_currentframe].get(), true, noTimeout);
	m_device->resetFences(m_frameFence[m_currentframe].get());

	VmaAllocation* ubo_Allocation = m_BufferPool.GetAllocation(m_ubo_buffer[m_currentframe]);
	{
		void* bufferMemory;
		vmaMapMemory(m_allocator.get(), *ubo_Allocation, &bufferMemory);
		Ubo_GlobalRenderData renderData;
		renderData.proj = camera.GetProjectionMatrix();
		renderData.view = camera.CalcViewMatrix();

		std::memcpy(bufferMemory, &renderData, sizeof(renderData));
		vmaUnmapMemory(m_allocator.get(), *ubo_Allocation);
	}


	uint32_t imageIndex;
	vk::Result aquireResult = m_device->acquireNextImageKHR(
		m_swapchain.swapchain.get(), noTimeout, m_imageAvailableSem[m_currentframe].get(), {}, &imageIndex);
	assert(aquireResult == vk::Result::eSuccess);

	m_device->resetCommandPool(m_graphicsPresentCommandPools[m_currentframe].get(), {});
	vk::CommandBuffer drawBuffer = m_graphicsPresentBuffer[m_currentframe].get();
	{
		vk::ClearValue clearValues[] = {
					vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }),
					vk::ClearDepthStencilValue(1.f, 0)
		};

		drawBuffer.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		drawBuffer.beginRenderPass(
			vk::RenderPassBeginInfo{}
			.setRenderPass(m_colorDepthRenderPass.get())
			.setFramebuffer(m_framebuffer[m_currentframe].get())
			.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), m_swapchain.extent))
			.setClearValueCount(GetSizeUint32(clearValues)).setPClearValues(clearValues),
			vk::SubpassContents::eInline);

		drawBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.get());

		gsl::span<const StaticSceneMesh> meshes = m_staticSceneData->GetMeshes();
		drawBuffer.bindIndexBuffer(m_staticSceneData->GetIndexBuffer(), meshes[0].firstIndex, StaticSceneData::IndexBufferIndexType);
		const vk::DeviceSize zeroOffset = 0;
		drawBuffer.bindVertexBuffers(0, m_staticSceneData->GetVertexBuffer(), zeroOffset);
		std::array<vk::DescriptorSet, 2> descriptorSets = {
			m_descriptorSet_texture,
			m_descriptorSet_ubo[m_currentframe]
		};

		drawBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSets, {});


		glm::vec3 positions[] = {
			glm::vec3(0),
			glm::vec3(0.f,1.f,0.f),
			glm::vec3(0.f,-1.f, 0.f),
			glm::vec3(-1.f,0.f,0.f),
			glm::vec3(-2.f,0.f,0.f),
			glm::vec3(0.f,0.f, 1.f)
		};
		for (const glm::vec3& pos : positions)
		{

			PushConstant_ModelMat pushConstant = {};
			pushConstant.model = glm::translate(glm::mat4(1), pos * 10.f);

			drawBuffer.pushConstants<PushConstant_ModelMat>(
				m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, pushConstant);

			drawBuffer.drawIndexed(meshes[0].indexCount, 1, 0, 0, 0);
		}
		drawBuffer.endRenderPass();
		drawBuffer.end();


		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		m_graphicsPresentQueues.submit(vk::SubmitInfo{}
			.setCommandBufferCount(1).setPCommandBuffers(&drawBuffer)
			.setWaitSemaphoreCount(1).setPWaitSemaphores(&(m_imageAvailableSem[m_currentframe].get())).setPWaitDstStageMask(waitStages)
			.setSignalSemaphoreCount(1).setPSignalSemaphores(&(m_renderFinishedSem[m_currentframe].get()))
			, m_frameFence[m_currentframe].get());

		m_graphicsPresentQueues.presentKHR(vk::PresentInfoKHR{}
			.setWaitSemaphoreCount(1).setPWaitSemaphores(&(m_renderFinishedSem[m_currentframe].get()))
			.setSwapchainCount(1).setPSwapchains(&(m_swapchain.swapchain.get())).setPImageIndices(&imageIndex)
		);
		m_currentframe = (m_currentframe + 1) % MaxInFlightFrames;

	}

}
