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
#include "ShaderSpecialisation.hpp"
#include "UniqueVmaMemoryMap.hpp"

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

	void RenderScene(vk::CommandBuffer commandBuffer, gsl::span<const MeshDataRef> Meshes, MeshDataManager& mdm)
	{

	}



	const vk::Format renderedDepthFormat = vk::Format::eR32Sfloat;
	constexpr int numPortals = 2;
	constexpr int numRecursions = 2;

	constexpr uint32_t cameraMatCount =  NTree::CalcTotalElements(numPortals, numRecursions);
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
	const char* additonalExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,

	};

	enabledExtensions.insert(enabledExtensions.end(), std::begin(additonalExtensions), std::end(additonalExtensions));


	assert(VulkanUtils::SupportsValidationLayers(enabledValidationLayers));

	m_vkInstance = CreateVulkanInstance("Vulkan Rasterizer", enabledValidationLayers, enabledExtensions);
	VulkanDebug::LoadDebugUtilsExtension(m_vkInstance.get());

	m_debugUtilsMessenger = DebugUtils::CreateDebugUtilsMessenger(m_vkInstance.get());
	m_surface = CreateSurface(m_vkInstance.get(), window);

	const char* deviceExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME,
	};

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
		stbi_uc* pixels = stbi_load("testTexture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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

	vk::Format preferedDepthFormats[] = { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint };
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
	// create rendered Depth buffer
//#error // use storage image!
	{
		vk::Format depthFormat = renderedDepthFormat;
		vk::DeviceSize texelSize = sizeof(float);


		VmaAllocationCreateInfo vmaInfo = {};
		vmaInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

		vk::DeviceSize storageTexelbufferSize = texelSize * m_swapchain.extent.height * m_swapchain.extent.width;//?
		vk::ImageCreateInfo renderedDepthBufferInfo = vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setArrayLayers(1)
			.setExtent(vk::Extent3D(m_swapchain.extent.width, m_swapchain.extent.height, 1))
			.setFormat(depthFormat)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setMipLevels(1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eInputAttachment);
		;

		for (UniqueVmaImage& renderedDepthImage : m_image_renderedDepth)
		{
			renderedDepthImage = UniqueVmaImage(m_allocator.get(), renderedDepthBufferInfo, vmaInfo);
		}



	}
	

	{
		for (size_t i = 0; i < m_imageview_renderedDepth.size(); ++i)
		{
	vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo{}
					.setImage(m_image_renderedDepth[i].Get())
.setViewType(vk::ImageViewType::e2D)
					.setFormat(renderedDepthFormat)
					.setSubresourceRange(vk::ImageSubresourceRange{}
						.setAspectMask(vk::ImageAspectFlagBits::eColor)
						.setBaseArrayLayer(0)
						.setLayerCount(1)
						.setBaseMipLevel(0)
						.setLevelCount(1))
					;
				m_imageview_renderedDepth[i] = m_device->createImageViewUnique(imageViewCreateInfo);

				
		}
	}
	m_colorDepthRenderPass = Renderpass::Portals_One_Pass_old(m_device.get(), m_swapchain.surfaceFormat.format, m_depthFormat);

	std::vector<std::string> debugRenderpass;
	auto testRenderpass = Renderpass::Portals_One_Pass(m_device.get(), m_swapchain.surfaceFormat.format, m_depthFormat, 2, 2, &debugRenderpass);
	{

		vk::FramebufferCreateInfo framebufferPrototype = vk::FramebufferCreateInfo{}
			.setWidth(m_swapchain.extent.width)
			.setHeight(m_swapchain.extent.height)
			.setRenderPass(m_colorDepthRenderPass.get())
			.setLayers(1);
		for (const vk::UniqueImageView& imageView : m_swapchain.imageViews)
		{
			vk::ImageView fbAttachments[] = {
				imageView.get(),
				m_depthBufferView.get(),
				m_imageview_renderedDepth[0].get(),
				m_imageview_renderedDepth[1].get() 
			};
			m_framebuffer.push_back(m_device->createFramebufferUnique(
				vk::FramebufferCreateInfo{ framebufferPrototype }
				.setAttachmentCount(GetSizeUint32(fbAttachments))
				.setPAttachments(fbAttachments))
			);
		}
	}

	m_vertShaderModule = VulkanUtils::CreateShaderModuleFromFile("shader.vert.spv", m_device.get());
	m_fragShaderModule = VulkanUtils::CreateShaderModuleFromFile("shader.frag.spv", m_device.get());
	m_fragShaderModule_subsequent = VulkanUtils::CreateShaderModuleFromFile("shader_subsequent.frag.spv", m_device.get());

	m_vertShaderModule_portal = VulkanUtils::CreateShaderModuleFromFile("portal.vert.spv", m_device.get());
	m_fragShaderModule_portal = VulkanUtils::CreateShaderModuleFromFile("portal.frag.spv", m_device.get());
	m_fragShaderModule_portal_subsequent = VulkanUtils::CreateShaderModuleFromFile("portal_subsequent.frag.spv", m_device.get());


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
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_cameraMat[] =
			{
				vk::DescriptorSetLayoutBinding{}
					.setBinding(0) // matches Shader code
					.setDescriptorType(vk::DescriptorType::eUniformBuffer)
					.setDescriptorCount(1)
					.setStageFlags(vk::ShaderStageFlagBits::eVertex),
			};

			vk::DescriptorSetLayoutCreateInfo descritorSetLayoutInfo_cameraMat = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_cameraMat))
				.setPBindings(descriptorSetBinding_cameraMat)
				;

			m_descriptorSetLayout_cameraMat = m_device->createDescriptorSetLayoutUnique(descritorSetLayoutInfo_cameraMat);
		}
		// rendered Depth storage texel buffer
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_renderedDepth[] = {
		vk::DescriptorSetLayoutBinding{}
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eInputAttachment)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment),

			};

			vk::DescriptorSetLayoutCreateInfo descritorSetLayoutInfo_renderedDepth = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_renderedDepth))
				.setPBindings(descriptorSetBinding_renderedDepth)
				;

			m_descriptorSetLayout_renderedDepth = m_device->createDescriptorSetLayoutUnique(descritorSetLayoutInfo_renderedDepth);
		}

		vk::DescriptorPoolSize descritproPoolSizes[] =
		{

		vk::DescriptorPoolSize()
			.setType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(1),

		vk::DescriptorPoolSize{}
			.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(GetSizeUint32(m_descriptorSet_ubo) + GetSizeUint32(m_descriptorSet_cameratMat)),


			vk::DescriptorPoolSize{}
			.setType(vk::DescriptorType::eInputAttachment)
			.setDescriptorCount(GetSizeUint32(m_descriptorSet_renderedDepth)),

		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo_sampler = vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(GetSizeUint32(descritproPoolSizes)).setPPoolSizes(descritproPoolSizes)
			.setMaxSets(7);


		m_descriptorPool = m_device->createDescriptorPoolUnique(descriptorPoolCreateInfo_sampler);

		{
			vk::DescriptorSetLayout layouts[] = {
				m_descriptorSetLayout_texture.get(),
				m_descriptorSetLayout_ubo.get(),
				m_descriptorSetLayout_ubo.get(), // we need two as we write it each frame, so we can leave one alone until its finished		
				m_descriptorSetLayout_renderedDepth.get(),
				m_descriptorSetLayout_renderedDepth.get(),
				m_descriptorSetLayout_cameraMat.get(),
				m_descriptorSetLayout_cameraMat.get(),
			};

			vk::DescriptorSetAllocateInfo descritproSetAllocateInfo = vk::DescriptorSetAllocateInfo{}
				.setDescriptorPool(m_descriptorPool.get())
				.setDescriptorSetCount(GetSizeUint32(layouts))
				.setPSetLayouts(layouts);


			std::vector<vk::DescriptorSet> descriptorSets = m_device->allocateDescriptorSets(descritproSetAllocateInfo);
			m_descriptorSet_texture = std::move(descriptorSets[0]);
			m_descriptorSet_ubo[0] = std::move(descriptorSets[1]);
			m_descriptorSet_ubo[1] = std::move(descriptorSets[2]);
			m_descriptorSet_renderedDepth[0] = std::move(descriptorSets[3]);
			m_descriptorSet_renderedDepth[1] = std::move(descriptorSets[4]);
			m_descriptorSet_cameratMat[0] = std::move(descriptorSets[5]);
				m_descriptorSet_cameratMat[1] = std::move(descriptorSets[6]);
		}

		// write texture descriptor set
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
		// write ubo descriptorSet
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

				m_ubo_buffer[i] = UniqueVmaBuffer(m_allocator.get(), bufferCreateInfo, allocCreateInfo);
			}


			std::array<vk::DescriptorBufferInfo, std::tuple_size_v<decltype(m_ubo_buffer)>> descriptorBufferInfos;
			std::array<vk::WriteDescriptorSet, std::tuple_size_v<decltype(m_ubo_buffer)>> writeDescriptorSets;

			for (size_t i = 0; i < descriptorBufferInfos.size(); ++i)
			{

				descriptorBufferInfos[i] = vk::DescriptorBufferInfo{}
					.setBuffer(m_ubo_buffer[i].Get())
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
// write cameraMat descriptorSet
		{
			constexpr vk::DeviceSize uniformBufferSize = sizeof(glm::mat4) * cameraMatCount;
			for (size_t i = 0; i < m_cameratMat_buffer.size(); ++i)
			{
				const vk::BufferCreateInfo bufferCreateInfo = vk::BufferCreateInfo{}
					.setSize(uniformBufferSize)
					.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
					.setSharingMode(vk::SharingMode::eExclusive);

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

				m_cameratMat_buffer[i] = UniqueVmaBuffer(m_allocator.get(), bufferCreateInfo, allocCreateInfo);
			}


			std::array<vk::DescriptorBufferInfo, std::tuple_size_v<decltype(m_cameratMat_buffer)>> descriptorBufferInfos;
			std::array<vk::WriteDescriptorSet, std::tuple_size_v<decltype(m_cameratMat_buffer)>> writeDescriptorSets;

			for (size_t i = 0; i < descriptorBufferInfos.size(); ++i)
			{

				descriptorBufferInfos[i] = vk::DescriptorBufferInfo{}
					.setBuffer(m_cameratMat_buffer[i].Get())
					.setOffset(0)
					.setRange(uniformBufferSize);


				writeDescriptorSets[i] = vk::WriteDescriptorSet{}
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType::eUniformBuffer)
					.setDstArrayElement(0)
					.setDstBinding(0) // matches shader code
					.setDstSet(m_descriptorSet_cameratMat[i])
					.setPBufferInfo(&descriptorBufferInfos[i])
					;
			}

			m_device->updateDescriptorSets(writeDescriptorSets, {});


		}

		// writed rendered Depth descriptor Set
		{
			std::array<vk::DescriptorImageInfo,2> imageInfos;
			std::array<vk::WriteDescriptorSet,2> writeDescriptorSet_inputAttachment;

			for (size_t i = 0; i < std::size(imageInfos); ++i)
			{

			imageInfos[i] = vk::DescriptorImageInfo{}
					.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setImageView(m_imageview_renderedDepth[i].get());

			writeDescriptorSet_inputAttachment[i] = vk::WriteDescriptorSet{}
				.setDescriptorCount(1)
				.setPImageInfo(&imageInfos[i])
				.setDescriptorType(vk::DescriptorType::eInputAttachment)
				.setDstArrayElement(0)
				.setDstBinding(0)
				.setDstSet(m_descriptorSet_renderedDepth[i])
				;

			}

			
			m_device->updateDescriptorSets(writeDescriptorSet_inputAttachment, {});

		}
	}

	vk::PushConstantRange pushConstantRange = vk::PushConstantRange{}
		.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.setOffset(0)
		.setSize(sizeof(PushConstant));


	vk::DescriptorSetLayout layouts[] = {
		m_descriptorSetLayout_texture.get(),
		m_descriptorSetLayout_ubo.get(),
		m_descriptorSetLayout_cameraMat.get(),
		m_descriptorSetLayout_renderedDepth.get(),
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutcreateInfo = vk::PipelineLayoutCreateInfo{}
		.setSetLayoutCount(GetSizeUint32(layouts)).setPSetLayouts(layouts)
		.setPushConstantRangeCount(1).setPPushConstantRanges(&pushConstantRange);

	m_pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutcreateInfo);

	const ShaderSpecialisation::MultiBytes<2> multibytes_disableRenderedDepth = []() {
		ShaderSpecialisation::MultiBytes<2> multibytes{};
		multibytes.data[0] = false; // disable rendered depth input a these are the initial shaders
		multibytes.data[1] = gsl::narrow<uint8_t>(cameraMatCount);
		return multibytes;
	}();

	const vk::SpecializationInfo disableRenderedDepth = ShaderSpecialisation::ReferenceMultibytes(multibytes_disableRenderedDepth);


	const ShaderSpecialisation::MultiBytes<2> multibytes_enableRenderedDepth = []() {
		ShaderSpecialisation::MultiBytes<2> multibytes{};
		multibytes.data[0] = false; // disable rendered depth input a these are the initial shaders
		multibytes.data[1] = gsl::narrow<uint8_t>(cameraMatCount);
		return multibytes;
	}();

	const vk::SpecializationInfo enableRenderedDepth = ShaderSpecialisation::ReferenceMultibytes(multibytes_enableRenderedDepth);

	{
		vk::PipelineShaderStageCreateInfo pipelineShaderStageCreationInfos[] =
		{
			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eVertex,
				m_vertShaderModule.get(),
				"main",
				&disableRenderedDepth),

			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eFragment,
				m_fragShaderModule.get(),
				"main",
				&disableRenderedDepth),
		};


		m_graphicsPipeline_scene_initial = GraphicsPipeline::CreateGraphicsPipeline_drawScene_initial(
			m_device.get(),
			m_swapchain.extent,
			m_colorDepthRenderPass.get(),
			m_pipelineLayout.get(),
			pipelineShaderStageCreationInfos);
	}
	{

		vk::PipelineShaderStageCreateInfo pipelineShaderStageCreationInfos[] =
		{
			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eVertex,
				m_vertShaderModule.get(),
				"main",
				&disableRenderedDepth),

			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eFragment,
				m_fragShaderModule_subsequent.get(),
				"main",
				&disableRenderedDepth),
		};


		m_graphicsPipeline_scene_subsequent = GraphicsPipeline::CreateGraphicsPipeline_drawScene_subsequent(
			m_device.get(),
			m_swapchain.extent,
			m_colorDepthRenderPass.get(),
			m_pipelineLayout.get(),
			pipelineShaderStageCreationInfos,
			2);
	}


	{
		vk::PipelineShaderStageCreateInfo pipelineShaderStageCreationInfos_portal[] =
		{
			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eVertex,
				m_vertShaderModule_portal.get(),
				"main",
				&enableRenderedDepth),

			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eFragment,
				m_fragShaderModule_portal.get(),
				"main",
				&enableRenderedDepth),
		};

		m_graphicsPipeline_portals_initial = GraphicsPipeline::CreateGraphicsPipeline_PortalRender_Initial(
			m_device.get(),
			m_swapchain.extent,
			m_colorDepthRenderPass.get(),
			m_pipelineLayout.get(),
			pipelineShaderStageCreationInfos_portal);


	}


	{

		vk::PipelineShaderStageCreateInfo shaderStage_scene_initial[] =
		{
			vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(m_vertShaderModule.get())
			.setPName("main"),

		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(m_fragShaderModule.get())
			.setPName("main"),

		};
	vk::PipelineShaderStageCreateInfo shaderStage_portal_initial[] =
		{
			vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(m_vertShaderModule_portal.get())
			.setPName("main"),

		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(m_fragShaderModule_portal.get())
			.setPName("main"),

		};
	vk::PipelineShaderStageCreateInfo shaderStage_scene_subsequent[] =
		{
			vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(m_vertShaderModule.get())
			.setPName("main"),

		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(m_fragShaderModule_subsequent.get())
			.setPName("main"),

		};
	vk::PipelineShaderStageCreateInfo shaderStage_portal_subsequent[] =
		{
			vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(m_vertShaderModule_portal.get())
			.setPName("main"),

		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(m_fragShaderModule_portal_subsequent.get())
			.setPName("main"),

		};


		GraphicsPipeline::PipelinesCreateInfo createInfo;
		createInfo.logicalDevice = m_device.get();
		createInfo.pipelineLayout = m_pipelineLayout.get();
		createInfo.renderpass = testRenderpass.get();
		createInfo.swapchainExtent = m_swapchain.extent;

		createInfo.pipelineShaderStageCreationInfos_sceneInitial = shaderStage_scene_initial;
		createInfo.pipelineShaderStageCreationInfos_sceneSubsequent = shaderStage_scene_subsequent;
		createInfo.pipelineShaderStageCreationInfos_portalInitial = shaderStage_portal_initial;
		createInfo.pipelineShaderStageCreationInfos_portalSubsequent = shaderStage_portal_subsequent;

		std::vector<std::string> debugPipelines;
		auto testPipelines = GraphicsPipeline::CreateGraphicPipelines(createInfo, 2, 2, &debugPipelines);

	}

	for (int i = 0; i < MaxInFlightFrames; ++i)
	{

		m_frameFence[i] = m_device->createFenceUnique(vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled));
		m_imageAvailableSem[i] = m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
		m_renderFinishedSem[i] = m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
	}

	m_meshData = std::make_unique<MeshDataManager>(m_allocator.get());

	const char* objsToLoad[] = { "torus.obj" , "sphere.obj" ,"cube.obj", "plane.obj", "halfSphere.obj" };
	// use graphics present queue to avoid ownership transfer
	m_meshData->LoadObjs(objsToLoad, m_device.get(), m_graphicsPresentCommandPools[0].get(), m_graphicsPresentQueues);
	enum ObjIdx
	{
		torusIdx = 0,
		sphereIdx,
		cubeIdx,
		planeIdx,
		halfSphereIdx
	};

	// Init Scene
	{

		m_scene = std::make_unique<Scene>(m_allocator.get());
		{
			const glm::vec3 torusPositions[] = {
				glm::vec3(0),
				glm::vec3(0.f,1.f,0.f),
				glm::vec3(0.f,-1.f, 0.f),
				glm::vec3(-6.f,10.f, -15.f),
				glm::vec3(6.f, 10.f , 15.f),
			};

			for (const glm::vec3& torusPos : torusPositions)
			{
				m_scene->Add(torusIdx, glm::translate(glm::mat4(1), torusPos));
			}
		}
		{
			const Transform floorTransform = Transform(glm::vec3(0.f, -5.f, 0.f), glm::vec3(100.f, 1.f, 100.f), glm::identity<glm::quat>());
			const glm::mat4 floorModelMat = floorTransform.ToMat();
			m_scene->Add(cubeIdx, floorModelMat);
		}
		{
			const glm::vec3 spherePos[] = {
					glm::vec3(-3.f,10.f, -5.f),
					glm::vec3(3.f, 10.f , 5.f),
			};

			for (const glm::vec3& p : spherePos)
			{
				m_scene->Add(sphereIdx, glm::translate(glm::mat4(1), p));
			}

		}
		{
			const glm::vec3 cubePos[] = {
							glm::vec3(-7.f,13.f, -9.f),
							glm::vec3(1.f, 10.f , -9.f),
			};

			for (const glm::vec3& p : cubePos)
			{
				m_scene->Add(cubeIdx, glm::translate(glm::mat4(1), p));
			}
		}

	}

	{
		const Transform portal_a(glm::vec3(0.f, 10.f, 0.f), glm::vec3(10.f, 10.f, 10.f), glm::angleAxis(glm::radians(90.0f), glm::vec3(1.f, 0.f, 0.f)));

		const Transform portal_a_to_b = Transform::FromTranslation(glm::vec3(0.f, 0.f, 30.f));

		m_portal = Portal::CreateWithTransformAndAtoB(halfSphereIdx, portal_a, portal_a_to_b);
	}

}

void GraphicsBackend::Render(const Camera& camera)
{
	constexpr uint64_t noTimeout = std::numeric_limits<uint64_t>::max();
	m_device->waitForFences(m_frameFence[m_currentframe].get(), true, noTimeout);
	m_device->resetFences(m_frameFence[m_currentframe].get());

	{
		VmaAllocation ubo_Allocation = m_ubo_buffer[m_currentframe].GetAllocation();
		UniqueVmaMemoryMap memoryMap(m_allocator.get(), ubo_Allocation);
		Ubo_GlobalRenderData renderData;
		renderData.proj = camera.GetProjectionMatrix();
		renderData.view = camera.m_transform.ToViewMat();

		std::memcpy(memoryMap.GetMappedMemoryPtr(), &renderData, sizeof(renderData));
	}

	{
		Transform cameraTransforms[cameraMatCount];
		Portal::CreateCameraTransforms(gsl::make_span(&m_portal, 1), camera.m_transform, numRecursions, cameraTransforms);

		glm::mat4 cameraViewMats[cameraMatCount];

		std::transform(std::begin(cameraTransforms), std::end(cameraTransforms), cameraViewMats, [](const Transform& e)
			{
				return e.ToViewMat();
			});

		VmaAllocation cameraMat_Allocation = m_cameratMat_buffer[m_currentframe].GetAllocation();
		UniqueVmaMemoryMap memoryMap(m_allocator.get(), cameraMat_Allocation);

		std::memcpy(memoryMap.GetMappedMemoryPtr(), &cameraViewMats, sizeof(cameraViewMats[0]) * std::size(cameraViewMats));
	}


	uint32_t imageIndex;
	vk::Result aquireResult = m_device->acquireNextImageKHR(
		m_swapchain.swapchain.get(), noTimeout, m_imageAvailableSem[m_currentframe].get(), {}, &imageIndex);
	assert(aquireResult == vk::Result::eSuccess);

	m_device->resetCommandPool(m_graphicsPresentCommandPools[m_currentframe].get(), {});
	vk::CommandBuffer drawBuffer = m_graphicsPresentBuffer[m_currentframe].get();
	{
		vk::ClearValue clearValues[] = {
					vk::ClearColorValue(std::array<float, 4>{ 100.f / 255.f, 149.f / 255.f, 237.f / 255.f, 1.f }),
					vk::ClearDepthStencilValue(1.f, 0),
					vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 0.f }),
					vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 0.f }),
		};

		drawBuffer.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		// render pass
		{
			const vk::ImageMemoryBarrier setRenderedImageAsColorAttachment = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(m_swapchain.images[imageIndex])
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseMipLevel(0)
					.setLevelCount(1)
					.setBaseArrayLayer(0)
					.setLayerCount(1))
				.setSrcAccessMask(vk::AccessFlags())
				.setDstAccessMask(vk::AccessFlagBits::eShaderWrite);

			;

			drawBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eFragmentShader,
				{}, {}, {}, setRenderedImageAsColorAttachment);


			// render Scene Subpass
			{
				drawBuffer.beginRenderPass(
					vk::RenderPassBeginInfo{}
					.setRenderPass(m_colorDepthRenderPass.get())
					.setFramebuffer(m_framebuffer[m_currentframe].get())
					.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), m_swapchain.extent))
					.setClearValueCount(GetSizeUint32(clearValues)).setPClearValues(clearValues),
					vk::SubpassContents::eInline);

				drawBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline_scene_initial.get());

				std::array<vk::DescriptorSet, 4> descriptorSets = {
					m_descriptorSet_texture,
					m_descriptorSet_ubo[m_currentframe],
					m_descriptorSet_cameratMat[m_currentframe],
					m_descriptorSet_renderedDepth[1],
				};

				drawBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSets, {});


				m_scene->Draw(*m_meshData, m_pipelineLayout.get(), drawBuffer, 0);


			}
			{
				// First Portal Pass
				drawBuffer.nextSubpass(vk::SubpassContents::eInline);
				drawBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline_portals_initial.get());

				// for now just bind it, we can use a different pipeline layout later
				{
					std::array<vk::DescriptorSet, 4> descriptorSets = {
						m_descriptorSet_texture,
						m_descriptorSet_ubo[m_currentframe],
						m_descriptorSet_cameratMat[m_currentframe],
						m_descriptorSet_renderedDepth[1],
					};

					drawBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSets, {});


				}

				// draw Portals
				{
					drawBuffer.bindIndexBuffer(m_meshData->GetIndexBuffer(), 0, MeshDataManager::IndexBufferIndexType);
					vk::DeviceSize vertexBufferOffset = 0;
					drawBuffer.bindVertexBuffers(0, m_meshData->GetVertexBuffer(), vertexBufferOffset);

					const MeshDataRef& portalMeshRef = m_meshData->GetMeshes()[m_portal.meshIndex];

					PushConstant pushConstant = {};
					pushConstant.model = m_portal.a_transform.ToMat();
					pushConstant.cameraIdx = 0;
					pushConstant.portalStencilVal = 1;
					pushConstant.debugColor = glm::vec4(0.5f, 0.f, 0.f, 1.f);

					drawBuffer.pushConstants<PushConstant>(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
					drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);

					pushConstant.model = m_portal.b_transform.ToMat();
					pushConstant.portalStencilVal = 2;
					pushConstant.debugColor = glm::vec4(0.0f, 0.f, 0.5f, 1.f);

					drawBuffer.pushConstants<PushConstant>(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
					drawBuffer.drawIndexed(portalMeshRef.indexCount, 1, portalMeshRef.firstIndex, 0, 1);


				}


				// Subsequent Scene Pass
				{

					vk::ClearAttachment clearDepthOnly = vk::ClearAttachment{}
						.setColorAttachment(1)
						.setAspectMask(vk::ImageAspectFlagBits::eDepth)
						.setClearValue(vk::ClearDepthStencilValue(1.f, 0));

					vk::ClearAttachment setRenderedDepthTo1 = vk::ClearAttachment{}
						.setColorAttachment(1)
						.setAspectMask(vk::ImageAspectFlagBits::eColor)
						.setClearValue(vk::ClearColorValue(std::array<float, 4>{ 1.f, 1.f, 1.f, 1.f}));



					vk::ClearRect wholeScreen(vk::Rect2D(vk::Offset2D(0, 0), m_swapchain.extent), 0, 1);

					std::array<vk::ClearAttachment, 2> clearAttachments = { clearDepthOnly,setRenderedDepthTo1, };
					std::array<vk::ClearAttachment, 1> clearAttachments1 = { clearDepthOnly, };

					drawBuffer.clearAttachments(clearAttachments1, wholeScreen);
					drawBuffer.nextSubpass(vk::SubpassContents::eInline);
					drawBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline_scene_subsequent.get());



					std::array<vk::DescriptorSet, 4> descriptorSets = {
										m_descriptorSet_texture,
										m_descriptorSet_ubo[m_currentframe],
										m_descriptorSet_cameratMat[m_currentframe],
										m_descriptorSet_renderedDepth[0],
					};

					drawBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSets, {});


					m_scene->Draw(*m_meshData, m_pipelineLayout.get(), drawBuffer, 1);



					// draw Camera
					{

						drawBuffer.bindIndexBuffer(m_meshData->GetIndexBuffer(), 0, MeshDataManager::IndexBufferIndexType);
						vk::DeviceSize vertexBufferOffset = 0;
						drawBuffer.bindVertexBuffers(0, m_meshData->GetVertexBuffer(), vertexBufferOffset);

						const MeshDataRef& cameraMeshRef = m_meshData->GetMeshes()[2];
						const MeshDataRef& cameraMeshRef2 = m_meshData->GetMeshes()[1];

						PushConstant pushConstant = {};
						Transform cameraTransform = camera.m_transform;
						pushConstant.model = cameraTransform.ToMat();
						pushConstant.cameraIdx = 1;
						pushConstant.portalStencilVal = 0;

						drawBuffer.pushConstants<PushConstant>(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
						drawBuffer.drawIndexed(cameraMeshRef.indexCount, 1, cameraMeshRef.firstIndex, 0, 1);



						cameraTransform.translation += glm::vec3(0.f, 1.f, 0.f);
						pushConstant.model = cameraTransform.ToMat();
						drawBuffer.pushConstants<PushConstant>(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
						drawBuffer.drawIndexed(cameraMeshRef2.indexCount, 1, cameraMeshRef2.firstIndex, 0, 1);

					}

				}

				// Subsequent Portal Pass
				//drawBuffer.nextSubpass(vk::SubpassContents::eInline);

			}
			drawBuffer.endRenderPass();
		}

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
