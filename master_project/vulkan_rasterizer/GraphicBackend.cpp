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
#include "RenderHelper.hpp"

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


	const vk::Format renderedDepthFormat = vk::Format::eR32Sfloat;

	const vk::Format renderedStencilFormat = vk::Format::eR8Sint;


}


void GraphicsBackend::Init(SDL_Window* window, int portalCount)
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
		.setSamplerAnisotropy(true)
		.setVertexPipelineStoresAndAtomics(true)
		.setFragmentStoresAndAtomics(true)
		;

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
		constexpr int rgbaPixelSize = 4;
		vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texWidth) * texHeight * rgbaPixelSize;


		vk::BufferCreateInfo stagingBufferInfo = vk::BufferCreateInfo{}
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
			.setSize(imageSize)
			.setSharingMode(vk::SharingMode::eExclusive);

		VmaAllocationCreateInfo vmaAllocInfo = {};
		vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
		UniqueVmaBuffer stageBuffer(m_allocator.get(), stagingBufferInfo, vmaAllocInfo);

		{
			UniqueVmaMemoryMap memoryMap(stageBuffer.GetAllocator(), stageBuffer.GetAllocation());
			std::memcpy(memoryMap.GetMappedMemoryPtr(), pixels, static_cast<size_t>(imageSize));
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

		m_textureImage = UniqueVmaImage(m_allocator.get(), imageCreateInfo, vmaAllocImage);


		VulkanDebug::SetObjectName(m_device.get(), m_textureImage.Get(), "texture Image");

		vk::UniqueCommandBuffer loadBuffer = CbUtils::AllocateSingle(m_device.get(), m_graphicsPresentCommandPools[0].get());
		loadBuffer->begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		{
			vk::ImageMemoryBarrier imageMemoryBarier = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(m_textureImage.Get())
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

			loadBuffer->copyBufferToImage(stageBuffer.Get(), m_textureImage.Get(), vk::ImageLayout::eTransferDstOptimal, bufferImageCopy);
		}
		{
			vk::ImageMemoryBarrier imageMemoryBarier1 = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setImage(m_textureImage.Get())
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

		vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo{}
			.setImage(m_textureImage.Get())
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eR8G8B8A8Unorm)
			.setSubresourceRange(vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1));

		m_textureImageView = m_device->createImageViewUnique(imageViewCreateInfo);


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

	}

	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	m_swapchain = Swapchain::Create(m_physicalDevice, m_device.get(), m_surface.get(), vk::Extent2D(windowWidth, windowHeight));

	vk::Format preferedDepthFormats[] = { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint };
	m_depthFormat = VulkanUtils::ChooseFormat(m_physicalDevice, preferedDepthFormats, vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	// Create Depth Stencil Buffer
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

		m_depthBuffer = UniqueVmaImage(m_allocator.get(), imageCreateInfo, vmaAllocImage);

		VulkanDebug::SetObjectName(m_device.get(), m_depthBuffer.Get(), "Depth Stencil Buffer");

		//---------------


		m_depthBufferView = m_device->createImageViewUnique(vk::ImageViewCreateInfo{}
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(m_depthFormat)
			.setImage(m_depthBuffer.Get())
			.setSubresourceRange(vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1)));

	}
	// create rendered Depth buffer
	{
		vk::DeviceSize texelSize = sizeof(float);


		VmaAllocationCreateInfo vmaInfo = {};
		vmaInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

		vk::ImageCreateInfo renderedDepthBufferInfo = vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setArrayLayers(1)
			.setExtent(vk::Extent3D(m_swapchain.extent.width, m_swapchain.extent.height, 1))
			.setFormat(renderedDepthFormat)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setMipLevels(1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eInputAttachment);
		;

		for (UniqueVmaImage& renderedDepthImage : m_image_renderedDepth)
		{
			renderedDepthImage = UniqueVmaImage(m_allocator.get(), renderedDepthBufferInfo, vmaInfo);
			std::string renderedDepthImageName = std::string("rendered depth") + std::to_string(std::distance(&m_image_renderedDepth[0], &renderedDepthImage));
			VulkanDebug::SetObjectName(m_device.get(), renderedDepthImage.Get(), renderedDepthImageName.c_str());
		}

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

	// create rendered stencil Buffer
	{
		vk::DeviceSize texelSize = sizeof(int8_t);


		VmaAllocationCreateInfo vmaInfo = {};
		vmaInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

		vk::ImageCreateInfo renderedStencilInfo = vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setArrayLayers(1)
			.setExtent(vk::Extent3D(m_swapchain.extent.width, m_swapchain.extent.height, 1))
			.setFormat(renderedStencilFormat)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setMipLevels(1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eInputAttachment);
		;

		for (UniqueVmaImage& renderedStencilImage : m_image_renderedStencil)
		{
			renderedStencilImage = UniqueVmaImage(m_allocator.get(), renderedStencilInfo, vmaInfo);
			std::string renderedStencilImageName = std::string("rendered stencil") + std::to_string(std::distance(&m_image_renderedStencil[0], &renderedStencilImage));
			VulkanDebug::SetObjectName(m_device.get(), renderedStencilImage.Get(), renderedStencilImageName.c_str());
		}

		for (size_t i = 0; i < m_image_renderedStencil.size(); ++i)
		{
			vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo{}
				.setImage(m_image_renderedStencil[i].Get())
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(renderedStencilFormat)
				.setSubresourceRange(vk::ImageSubresourceRange{}
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseArrayLayer(0)
					.setLayerCount(1)
					.setBaseMipLevel(0)
					.setLevelCount(1))
				;
			m_imageview_renderedStencil[i] = m_device->createImageViewUnique(imageViewCreateInfo);
		}
	}


	std::vector<std::string> debugRenderpass;
	m_portalRenderPass = Renderpass::Portals_One_Pass_dynamicState(m_device.get(), m_swapchain.surfaceFormat.format,
		m_depthFormat, renderedDepthFormat, renderedStencilFormat, recursionCount, &debugRenderpass);

	// create Framebuffer
	{

		vk::FramebufferCreateInfo framebufferPrototype = vk::FramebufferCreateInfo{}
			.setWidth(m_swapchain.extent.width)
			.setHeight(m_swapchain.extent.height)
			.setRenderPass(m_portalRenderPass.get())
			.setLayers(1);
		for (const vk::UniqueImageView& imageView : m_swapchain.imageViews)
		{
			vk::ImageView fbAttachments[] = {
				imageView.get(),
				m_depthBufferView.get(),
				m_imageview_renderedDepth[0].get(),
				m_imageview_renderedDepth[1].get(),
				m_imageview_renderedStencil[0].get(),
				m_imageview_renderedStencil[1].get(),
			};
			m_framebuffer.push_back(m_device->createFramebufferUnique(
				vk::FramebufferCreateInfo{ framebufferPrototype }
				.setAttachmentCount(GetSizeUint32(fbAttachments))
				.setPAttachments(fbAttachments))
			);
		}
	}

	// create Shader Modules
	{
		m_vertShaderModule = VulkanUtils::CreateShaderModuleFromFile("shader.vert.spv", m_device.get());
		m_fragShaderModule = VulkanUtils::CreateShaderModuleFromFile("shader.frag.spv", m_device.get());
		m_fragShaderModule_subsequent = VulkanUtils::CreateShaderModuleFromFile("shader_subsequent.frag.spv", m_device.get());

		m_vertShaderModule_portal = VulkanUtils::CreateShaderModuleFromFile("portal.vert.spv", m_device.get());
		m_fragShaderModule_portal = VulkanUtils::CreateShaderModuleFromFile("portal.frag.spv", m_device.get());
		m_fragShaderModule_portal_subsequent = VulkanUtils::CreateShaderModuleFromFile("portal_subsequent.frag.spv", m_device.get());
	}

	const int cameraMatElements = PortalManager::GetCameraBufferElementCount(recursionCount, portalCount);

	m_stencilRefTree.RecalcTree(maxVisiblePortalsForRecursion);

	// Creating Descriptor Set Buffers
	{

		for (size_t i = 0; i < MaxInFlightFrames; ++i)
		{
			std::string indexAsString = std::to_string(i);

			{
				const vk::BufferCreateInfo cameraMatBufferCreateInfo = vk::BufferCreateInfo{}
					.setSize(cameraMatElements * sizeof(glm::mat4))
					.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
					.setSharingMode(vk::SharingMode::eExclusive);

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

				m_cameratMat_buffer[i] = UniqueVmaBuffer(m_allocator.get(), cameraMatBufferCreateInfo, allocCreateInfo);
				VulkanDebug::SetObjectName(m_device.get(), m_cameratMat_buffer[i].Get(), (std::string("camera mat") + indexAsString).c_str());
			}

			{
				const vk::BufferCreateInfo cameraIndexBufferCreateInfo = vk::BufferCreateInfo{}
					.setSize(m_stencilRefTree.GetCameraIndexBufferElementCount() * sizeof(uint32_t))
					.setUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst)
					.setSharingMode(vk::SharingMode::eExclusive);

				VmaAllocationCreateInfo cameraIndexBufferAllocCreateInfo = {};
				cameraIndexBufferAllocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

				m_cameraIndexBuffer[i] = UniqueVmaBuffer(m_allocator.get(), cameraIndexBufferCreateInfo, cameraIndexBufferAllocCreateInfo);
				VulkanDebug::SetObjectName(m_device.get(), m_cameraIndexBuffer[i].Get(), (std::string("camera index") + indexAsString).c_str());
			}
			{
				const vk::BufferCreateInfo portalIdxHelperCreateInfo = vk::BufferCreateInfo{}
					.setSize(PortalManager::GetPortalIndexHelperElementCount(recursionCount, portalCount) * sizeof(uint32_t))
					.setUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst)
					.setSharingMode(vk::SharingMode::eExclusive);

				VmaAllocationCreateInfo portalIdxHelperBufferAllocCreateInfo = {};
				portalIdxHelperBufferAllocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;

				m_portalIndexHelperBuffer[i] = UniqueVmaBuffer(m_allocator.get(), portalIdxHelperCreateInfo, portalIdxHelperBufferAllocCreateInfo);
				VulkanDebug::SetObjectName(m_device.get(), m_portalIndexHelperBuffer[i].Get(), (std::string("portal idx helper") + indexAsString).c_str());
			}
			{
				const vk::BufferCreateInfo uboBufferCreateInfo = vk::BufferCreateInfo{}
					.setSize(sizeof(Ubo_GlobalRenderData))
					.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
					.setSharingMode(vk::SharingMode::eExclusive);

				VmaAllocationCreateInfo uboAllocCreateInfo = {};
				uboAllocCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;

				m_ubo_buffer[i] = UniqueVmaBuffer(m_allocator.get(), uboBufferCreateInfo, uboAllocCreateInfo);
				VulkanDebug::SetObjectName(m_device.get(), m_ubo_buffer[i].Get(), (std::string("ubo buffer") + indexAsString).c_str());
			}


		}
	}


	// Descriptor Set layouts - Combined Image Sampler
	{
		// Texture
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_texture[] = {
				vk::DescriptorSetLayoutBinding{}
					.setBinding(0)
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
					.setStageFlags(vk::ShaderStageFlagBits::eFragment),
			};

			vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo_texture = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_texture))
				.setPBindings(descriptorSetBinding_texture)
				;

			m_descriptorSetLayout_texture = m_device->createDescriptorSetLayoutUnique(descriptorSetLayoutInfo_texture);
		}

		// Ubo
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_ubo[] =
			{
				vk::DescriptorSetLayoutBinding{}
					.setBinding(0) // matches Shader code
					.setDescriptorType(vk::DescriptorType::eUniformBuffer)
					.setDescriptorCount(1)
					.setStageFlags(vk::ShaderStageFlagBits::eVertex),
			};

			vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo_ubo = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_ubo))
				.setPBindings(descriptorSetBinding_ubo)
				;

			m_descriptorSetLayout_ubo = m_device->createDescriptorSetLayoutUnique(descriptorSetLayoutInfo_ubo);
		}

		// camera mats
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_cameraMat[] =
			{
				vk::DescriptorSetLayoutBinding{}
					.setBinding(0) // matches Shader code
					.setDescriptorType(vk::DescriptorType::eUniformBuffer)
					.setDescriptorCount(1)
					.setStageFlags(vk::ShaderStageFlagBits::eVertex),
			};

			vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo_cameraMat = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_cameraMat))
				.setPBindings(descriptorSetBinding_cameraMat)
				;

			m_descriptorSetLayout_cameraMat = m_device->createDescriptorSetLayoutUnique(descriptorSetLayoutInfo_cameraMat);
		}
		// camera indices
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_cameraIndices[] =
			{
				vk::DescriptorSetLayoutBinding{}
					.setBinding(0) // matches Shader code
					.setDescriptorType(vk::DescriptorType::eStorageBuffer)
					.setDescriptorCount(1)
					.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment),
			};

			vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo_cameraIndices = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_cameraIndices))
				.setPBindings(descriptorSetBinding_cameraIndices)
				;

			m_descriptorSetLayout_cameraIndices = m_device->createDescriptorSetLayoutUnique(descriptorSetLayoutInfo_cameraIndices);
		}
		// portal index helper
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_portalIndexHelper[] =
			{
				vk::DescriptorSetLayoutBinding{}
					.setBinding(0) // matches Shader code
					.setDescriptorType(vk::DescriptorType::eStorageBuffer)
					.setDescriptorCount(1)
					.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment),
			};

			vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo_portalIndexHelper = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_portalIndexHelper))
				.setPBindings(descriptorSetBinding_portalIndexHelper)
				;

			m_descriptorSetLayout_portalIndexHelper = m_device->createDescriptorSetLayoutUnique(descriptorSetLayoutInfo_portalIndexHelper);
		}


		// rendered  input attachment
		{
			vk::DescriptorSetLayoutBinding descriptorSetBinding_rendered[] = {

				// depth
				vk::DescriptorSetLayoutBinding{}
					.setBinding(0)
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType::eInputAttachment)
					.setStageFlags(vk::ShaderStageFlagBits::eFragment),

				// stencil
				vk::DescriptorSetLayoutBinding{}
					.setBinding(1)
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType::eInputAttachment)
					.setStageFlags(vk::ShaderStageFlagBits::eFragment),


			};

			vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo_rendered = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(GetSizeUint32(descriptorSetBinding_rendered))
				.setPBindings(descriptorSetBinding_rendered)
				;

			m_descriptorSetLayout_rendered = m_device->createDescriptorSetLayoutUnique(descriptorSetLayoutInfo_rendered);
		}
		
	
	}

	// create Descriptor pool
	{
		vk::DescriptorPoolSize descritproPoolSizes[] =
		{

		vk::DescriptorPoolSize()
			.setType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(1),

		vk::DescriptorPoolSize{}
			.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(
				GetSizeUint32(m_descriptorSet_ubo)
				+ GetSizeUint32(m_descriptorSet_cameratMat)
				+ GetSizeUint32(m_descriptorSet_cameraIndices)
				+ GetSizeUint32(m_descriptorSet_portalIndexHelper)

			),

			vk::DescriptorPoolSize{}
			.setType(vk::DescriptorType::eInputAttachment)
			.setDescriptorCount(GetSizeUint32(m_descriptorSet_rendered) * 2),

		};

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo_sampler = vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(GetSizeUint32(descritproPoolSizes)).setPPoolSizes(descritproPoolSizes)
			.setMaxSets(
				1 // combined image sampler
				+ GetSizeUint32(m_descriptorSet_ubo)
				+ GetSizeUint32(m_descriptorSet_cameratMat)
				+ GetSizeUint32(m_descriptorSet_cameraIndices)
				+ GetSizeUint32(m_descriptorSet_portalIndexHelper)
				+ GetSizeUint32(m_descriptorSet_rendered)
			);


		m_descriptorPool = m_device->createDescriptorPoolUnique(descriptorPoolCreateInfo_sampler);
	}


	// create descriptor Sets
	{
		vk::DescriptorSetLayout layouts[] = {
			m_descriptorSetLayout_texture.get(),
			m_descriptorSetLayout_ubo.get(),
			m_descriptorSetLayout_ubo.get(), // we need two as we write it each frame, so we can leave one alone until its finished		
			m_descriptorSetLayout_rendered.get(),
			m_descriptorSetLayout_rendered.get(),
			m_descriptorSetLayout_cameraMat.get(),
			m_descriptorSetLayout_cameraMat.get(),
			m_descriptorSetLayout_cameraIndices.get(),
			m_descriptorSetLayout_cameraIndices.get(),
			m_descriptorSetLayout_portalIndexHelper.get(),
			m_descriptorSetLayout_portalIndexHelper.get(),
		};

		vk::DescriptorSetAllocateInfo descritproSetAllocateInfo = vk::DescriptorSetAllocateInfo{}
			.setDescriptorPool(m_descriptorPool.get())
			.setDescriptorSetCount(GetSizeUint32(layouts))
			.setPSetLayouts(layouts);


		std::vector<vk::DescriptorSet> descriptorSets = m_device->allocateDescriptorSets(descritproSetAllocateInfo);
		m_descriptorSet_texture = std::move(descriptorSets[0]);
		m_descriptorSet_ubo[0] = std::move(descriptorSets[1]);
		m_descriptorSet_ubo[1] = std::move(descriptorSets[2]);
		m_descriptorSet_rendered[0] = std::move(descriptorSets[3]);
		m_descriptorSet_rendered[1] = std::move(descriptorSets[4]);
		m_descriptorSet_cameratMat[0] = std::move(descriptorSets[5]);
		m_descriptorSet_cameratMat[1] = std::move(descriptorSets[6]);
		m_descriptorSet_cameraIndices[0] = std::move(descriptorSets[7]);
		m_descriptorSet_cameraIndices[1] = std::move(descriptorSets[8]);
		m_descriptorSet_portalIndexHelper[0] = std::move(descriptorSets[9]);
		m_descriptorSet_portalIndexHelper[1] = std::move(descriptorSets[10]);
	}

	// write descriptor sets
	{
		// write texture descriptor set
		{

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


		auto updateDescriptorSetsBuffers = [](
			vk::Device device,
			std::array<UniqueVmaBuffer, MaxInFlightFrames>& buffer,
			std::array<vk::DescriptorSet, MaxInFlightFrames>& descriptorSet,
			vk::DeviceSize bufferSize, vk::DescriptorType type, int binding)
		{
			std::array<vk::DescriptorBufferInfo, MaxInFlightFrames> descriptorBufferInfos;
			std::array<vk::WriteDescriptorSet, MaxInFlightFrames> writeDescriptorSets;

			for (size_t i = 0; i < MaxInFlightFrames; ++i)
			{

				descriptorBufferInfos[i] = vk::DescriptorBufferInfo{}
					.setBuffer(buffer[i].Get())
					.setOffset(0)
					.setRange(bufferSize);


				writeDescriptorSets[i] = vk::WriteDescriptorSet{}
					.setDescriptorCount(1)
					.setDescriptorType(type)
					.setDstArrayElement(0)
					.setDstBinding(0) // matches shader code
					.setDstSet(descriptorSet[i])
					.setPBufferInfo(&descriptorBufferInfos[i])
					;
			}

			device.updateDescriptorSets(writeDescriptorSets, {});
		};

		// write ubo buffer descriptor set
		updateDescriptorSetsBuffers(m_device.get(), m_ubo_buffer, m_descriptorSet_ubo, sizeof(Ubo_GlobalRenderData),
			vk::DescriptorType::eUniformBuffer, 0 /*matches shader code*/);

		// write camera mat descriptor set
		updateDescriptorSetsBuffers(m_device.get(), m_cameratMat_buffer, m_descriptorSet_cameratMat,
			PortalManager::GetCameraBufferElementCount(recursionCount, portalCount) * sizeof(glm::mat4), vk::DescriptorType::eUniformBuffer, 0 /*matches shader code*/);

		// write camera index descriptor set
		updateDescriptorSetsBuffers(m_device.get(), m_cameraIndexBuffer, m_descriptorSet_cameraIndices,
			m_stencilRefTree.GetCameraIndexBufferElementCount() * sizeof(uint32_t),
			vk::DescriptorType::eStorageBuffer,0 /*matches shader code*/);

		// write portal index helper descriptor set
		updateDescriptorSetsBuffers(m_device.get(), m_portalIndexHelperBuffer, m_descriptorSet_portalIndexHelper,
			PortalManager::GetPortalIndexHelperElementCount(recursionCount, portalCount) * sizeof(uint32_t),vk::DescriptorType::eStorageBuffer,0 /*matches shader code*/);


		// write rendered Depth descriptor Set
		{
			std::array<vk::DescriptorImageInfo, 2> imageInfos;
			std::array<vk::WriteDescriptorSet, 2> writeDescriptorSet_inputAttachment;

			// depth
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
					.setDstSet(m_descriptorSet_rendered[i])
					;

			}


			m_device->updateDescriptorSets(writeDescriptorSet_inputAttachment, {});
			
			// stencil
			for (size_t i = 0; i < std::size(imageInfos); ++i)
			{

				imageInfos[i] = vk::DescriptorImageInfo{}
					.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setImageView(m_imageview_renderedStencil[i].get());

				writeDescriptorSet_inputAttachment[i] = vk::WriteDescriptorSet{}
					.setDescriptorCount(1)
					.setPImageInfo(&imageInfos[i])
					.setDescriptorType(vk::DescriptorType::eInputAttachment)
					.setDstArrayElement(0)
					.setDstBinding(1)
					.setDstSet(m_descriptorSet_rendered[i])
					;
			}

			m_device->updateDescriptorSets(writeDescriptorSet_inputAttachment, {});


		}
	}

	// create Pipeline Layout
	{
		vk::PushConstantRange pushConstantRange = vk::PushConstantRange{}
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
			.setOffset(0)
			.setSize(sizeof(PushConstant));


		vk::DescriptorSetLayout layouts[] = {
			m_descriptorSetLayout_texture.get(),
			m_descriptorSetLayout_ubo.get(),
			m_descriptorSetLayout_cameraMat.get(),
			m_descriptorSetLayout_rendered.get(),
			m_descriptorSetLayout_cameraIndices.get(),
			m_descriptorSetLayout_portalIndexHelper.get(),
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutcreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(GetSizeUint32(layouts)).setPSetLayouts(layouts)
			.setPushConstantRangeCount(1).setPPushConstantRanges(&pushConstantRange);

		m_pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutcreateInfo);
	}

	m_meshData = std::make_unique<MeshDataManager>(m_allocator.get());

	const char* objsToLoad[ObjectIds::enum_size] = { "torus.obj" , "sphere.obj" ,"cube.obj", "plane.obj", "halfSphere.obj", "inverted_cube.obj" };
	// use graphics present queue to avoid ownership transfer
	m_meshData->LoadObjs(objsToLoad, m_device.get(), m_graphicsPresentCommandPools[0].get(), m_graphicsPresentQueues);

	for (const char* obj : objsToLoad)
	{
		m_triangleMeshes.emplace_back();
		m_triangleMeshes.back() = TriangleMesh::FromFile(obj);
	}

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
			
			const glm::vec4 debugColors[] =
			{
				glm::vec4(1.f,1.f,1.f,1.f),
				glm::vec4(1.f,1.f,1.f,1.f),
				glm::vec4(1.f,1.f,1.f,1.f),
				glm::vec4(0.f,0.f,0.f,1.f),
				glm::vec4(0.33f,0.33f,0.33f,1.f),
				glm::vec4(.66f,.66f,.66f,1.f),
			};


			for (int i = 0; i < std::size(torusPositions); ++i)
			{
				m_scene->Add(ObjectIds::torusIdx, glm::translate(glm::mat4(1), torusPositions[i]), debugColors[i]);
			}
		}
		{
			Transform floorTransform = Transform(glm::vec3(0.f, -5.f, 0.f), glm::vec3(100.f, 1.f, 100.f), glm::identity<glm::quat>());
			m_scene->Add(ObjectIds::cubeIdx, floorTransform.ToMat());

			floorTransform.translation.y += 80.f;

			m_scene->Add(ObjectIds::cubeIdx, floorTransform.ToMat());
		}

		// perspective warped cube
		{
			glm::mat4 perspectiveMat(1.f);

			perspectiveMat[1][1] = 2.f;
			perspectiveMat[1][3] = 1.f;
			perspectiveMat[3][1] = +1.f;
			perspectiveMat[3][3] = 1.f;

			Transform trans(glm::vec3(-5.f, 5.f, 0.f), 3.f, glm::identity<glm::quat>());


			m_scene->Add(ObjectIds::cubeIdx, trans.ToMat() * perspectiveMat);
		}

		{
			Transform enclosingCube = Transform(glm::vec3{}, 150.f, glm::identity<glm::quat>());
			//m_scene->Add(invertedCubeIdx, enclosingCube.ToMat());
		}

		{
			const glm::vec3 spherePos[] = {
					glm::vec3(-3.f,10.f, -5.f),
					glm::vec3(3.f, 10.f , 5.f),
			};
	
			const glm::vec4 debugColors[] =
			{
				glm::vec4(1.f,0.33f,0.33f,1.f),
				glm::vec4(1.f,.66f,.66f,1.f),
			};

			for (int i = 0; i < std::size(spherePos); ++i)
			{
				m_scene->Add(ObjectIds::sphereIdx, glm::translate(glm::mat4(1), spherePos[i]), debugColors[i]);
			}

		}
		{
			const glm::vec3 cubePos[] = {
							glm::vec3(-7.f,13.f, -9.f),
							glm::vec3(1.f, 10.f , -9.f),
			};
		
			const glm::vec4 debugColors[] =
			{
				glm::vec4(.66f,0.33f,0.99f,1.f),
				glm::vec4(.33f,.66f,.99f,1.f),
			};

			for (int i = 0; i < std::size(cubePos); ++i)
			{
				m_scene->Add(ObjectIds::cubeIdx, glm::translate(glm::mat4(1), cubePos[i]), debugColors[i]);
			}
		}

		{

			const glm::vec4 debugColors[] =
			{

				glm::vec4(0.75f, 0.25f, 0.f, 1.f),
				glm::vec4(0.5f, 0.5f, 0.f, 1.f),
				glm::vec4(0.25f, 0.75f, 0.f, 1.f),
				glm::vec4(0.f, 1.f, 0.f, 1.f),
				glm::vec4(0.f, 0.75f, 0.25f, 1.f),
				glm::vec4(0.f, 0.5f, 0.5f, 1.f),
				glm::vec4(0.f, 0.25f, 0.75f, 1.f),
				glm::vec4(0.f, 0.f, 1.f, 1.f),
				glm::vec4(0.25f, 0.f, 1.f, 0.75f),
				glm::vec4(0.5f, 0.f, 1.f, 0.5f),
				glm::vec4(0.75f, 0.f, 1.f, 0.25f),

			};


			int count = 8;
			const glm::vec3 baseTranslation = glm::vec3(0.f, 0.f, 50.f);
			for (int i = 0; i < count; ++i)
			{
				const glm::vec3 trans = glm::rotate(glm::angleAxis(glm::radians( (360.f / 8) * i), glm::vec3(0.f, 1.f, 0.f)), baseTranslation);


				Transform transf(trans, glm::vec3(3.f, 10.f, 3.f), glm::identity<glm::quat>());
				m_scene->Add(ObjectIds::cubeIdx, transf.ToMat(), debugColors[i]);
			}


		}

	}

	{
		const Transform portal_a(glm::vec3(0.f, 10.f, 0.f), 10.f, glm::angleAxis(glm::radians(0.0f), glm::vec3(1.f, 0.f, 0.f)));

		const Transform portal_b = Transform(glm::vec3(5.f, 10.f, 30.f), 10.f, glm::angleAxis(glm::radians(0.0f), glm::vec3(1.f, 0.f, 0.f)));

		m_portalManager.Add(Portal::CreateWithPortalTransforms(ObjectIds::halfSphereIdx, portal_a.ToMat(), portal_b.ToMat()));
	}
	{
		const Transform portal_a(glm::vec3(30.f, 10.f, 0.f), 10.f, glm::angleAxis(glm::radians(90.0f), glm::vec3(1.f, 0.f, 0.f)));

		const Transform portal_b = Transform(glm::vec3(35.f, 15.f, 20.f), 10.f, glm::angleAxis(glm::radians(45.0f), glm::vec3(1.f, 0.f, 0.f)));

		m_portalManager.Add(Portal::CreateWithPortalTransforms(ObjectIds::planeIdx, portal_a.ToMat(), portal_b.ToMat()));
	}

	// create Graphic pipelines
	{
		enum specialisationConstantId
		{
			max_portal_count_cid = 0,
			camera_mat_count_cid = 1,

			enum_size_specialisationId,
		};


		const ShaderSpecialisation::MultiBytes<enum_size_specialisationId> multibytes_camerMats = [this, expectedPortalCount]() {
			ShaderSpecialisation::MultiBytes<enum_size_specialisationId> multibytes{};
			multibytes.data[max_portal_count_cid] = expectedPortalCount;// gsl::narrow<uint8_t>(m_portalManager.GetPortalCount());
			multibytes.data[camera_mat_count_cid] = gsl::narrow<uint8_t>(m_portalManager.GetCameraBufferElementCount(recursionCount));
			return multibytes;
		}();

		const vk::SpecializationInfo setCameraMats = ShaderSpecialisation::ReferenceMultibytes(multibytes_camerMats);

		vk::PipelineShaderStageCreateInfo shaderStage_scene_initial[] =
		{
			vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(m_vertShaderModule.get())
			.setPName("main")
			.setPSpecializationInfo(&setCameraMats),


		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(m_fragShaderModule.get())
			.setPName("main")
			.setPSpecializationInfo(&setCameraMats),

		};
		vk::PipelineShaderStageCreateInfo shaderStage_portal_initial[] =
		{
			vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(m_vertShaderModule_portal.get())
			.setPName("main")
			.setPSpecializationInfo(&setCameraMats),

		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(m_fragShaderModule_portal.get())
			.setPName("main")
			.setPSpecializationInfo(&setCameraMats),

		};
		vk::PipelineShaderStageCreateInfo shaderStage_scene_subsequent[] =
		{
			vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(m_vertShaderModule.get())
			.setPName("main")
			.setPSpecializationInfo(&setCameraMats),

		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(m_fragShaderModule_subsequent.get())
			.setPName("main")
			.setPSpecializationInfo(&setCameraMats),

		};
		vk::PipelineShaderStageCreateInfo shaderStage_portal_subsequent[] =
		{
			vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(m_vertShaderModule_portal.get())
			.setPName("main")
			.setPSpecializationInfo(&setCameraMats),

		vk::PipelineShaderStageCreateInfo{}
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(m_fragShaderModule_portal_subsequent.get())
			.setPName("main")
			.setPSpecializationInfo(&setCameraMats),

		};


		GraphicsPipeline::PipelinesCreateInfo createInfo;
		createInfo.logicalDevice = m_device.get();
		createInfo.pipelineLayout = m_pipelineLayout.get();
		createInfo.renderpass = m_portalRenderPass.get();
		createInfo.swapchainExtent = m_swapchain.extent;

		createInfo.pipelineShaderStageCreationInfos_sceneInitial = shaderStage_scene_initial;
		createInfo.pipelineShaderStageCreationInfos_sceneSubsequent = shaderStage_scene_subsequent;
		createInfo.pipelineShaderStageCreationInfos_portalInitial = shaderStage_portal_initial;
		createInfo.pipelineShaderStageCreationInfos_portalSubsequent = shaderStage_portal_subsequent;

		std::vector<std::string> debugPipelines;
		m_graphicPipelines = GraphicsPipeline::CreateGraphicPipelines_dynamicState(
			createInfo, recursionCount, &debugPipelines);

	}

	for (int i = 0; i < MaxInFlightFrames; ++i)
	{

		m_frameFence[i] = m_device->createFenceUnique(vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled));
		m_imageAvailableSem[i] = m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
		m_renderFinishedSem[i] = m_device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
	}

	assert(m_portalManager.GetPortalCount() == expectedPortalCount);
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

		std::memcpy(memoryMap.GetMappedMemoryPtr(), &renderData, sizeof(renderData));
	}

	{
		const int cameraBufferElementCount = m_portalManager.GetCameraBufferElementCount(recursionCount);

		std::vector<glm::mat4> cameraViewMats;
		cameraViewMats.resize(cameraBufferElementCount);

		m_portalManager.CreateCameraMats(camera.m_transform.ToMat(), recursionCount, cameraViewMats);
		std::transform(std::begin(cameraViewMats), std::end(cameraViewMats), std::begin(cameraViewMats), [](const glm::mat4& e)
			{
				return glm::inverse(e);
			});


		VmaAllocation cameraMat_Allocation = m_cameratMat_buffer[m_currentframe].GetAllocation();
		UniqueVmaMemoryMap memoryMap(m_allocator.get(), cameraMat_Allocation);

		std::memcpy(memoryMap.GetMappedMemoryPtr(), std::data(cameraViewMats), sizeof(cameraViewMats[0]) * std::size(cameraViewMats));
	}


	uint32_t imageIndex;
	vk::Result aquireResult = m_device->acquireNextImageKHR(
		m_swapchain.swapchain.get(), noTimeout, m_imageAvailableSem[m_currentframe].get(), {}, &imageIndex);
	assert(aquireResult == vk::Result::eSuccess);

	m_device->resetCommandPool(m_graphicsPresentCommandPools[m_currentframe].get(), {});
	vk::CommandBuffer drawBuffer = m_graphicsPresentBuffer[m_currentframe].get();

	// drawing
	{
		vk::ClearValue clearValues[] = {
			// output
			vk::ClearColorValue(std::array<float, 4>{ 100.f / 255.f, 149.f / 255.f, 237.f / 255.f, 1.f }),
			// depth Stencil
			vk::ClearDepthStencilValue(1.f, 0),
			// rendered depth x 2
			vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 0.f }),
			vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 0.f }),
			// rendered stencil x 2
			vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 0.f }),
			vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 0.f }),
		};

		drawBuffer.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		// clear the helper buffer
		drawBuffer.fillBuffer(m_portalIndexHelperBuffer[m_currentframe].Get(), 0,
			m_portalManager.GetPortalIndexHelperElementCount(recursionCount) * sizeof(uint32_t), 0);

		// set all values of camera index buffer to all 1s, so we can find invalid indices
		drawBuffer.fillBuffer(m_cameraIndexBuffer[m_currentframe].Get(), 0,
			m_stencilRefTree.GetCameraIndexBufferElementCount() * sizeof(uint32_t), ~(uint32_t(0)));

		// render pass
		{

			// initial / iteration 0
			{

				const int renderedInputIdx = 1;
				// render Scene Subpass
				{
					drawBuffer.beginRenderPass(
						vk::RenderPassBeginInfo{}
						.setRenderPass(m_portalRenderPass.get())
						.setFramebuffer(m_framebuffer[m_currentframe].get())
						.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), m_swapchain.extent))
						.setClearValueCount(GetSizeUint32(clearValues)).setPClearValues(clearValues),
						vk::SubpassContents::eInline);

					drawBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicPipelines[0].get());

					std::array<vk::DescriptorSet, 6> descriptorSets = {
						m_descriptorSet_texture,
						m_descriptorSet_ubo[m_currentframe],
						m_descriptorSet_cameratMat[m_currentframe],
						m_descriptorSet_rendered[renderedInputIdx],
						m_descriptorSet_cameraIndices[m_currentframe],
						m_descriptorSet_portalIndexHelper[m_currentframe],

					};

					drawBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSets, {});


					constexpr int cameraIndex = 0;
					m_scene->Draw(*m_meshData, m_pipelineLayout.get(), drawBuffer, cameraIndex, 0);
				}


				// First Portal Pass
				{
					constexpr int iterationElementIndex = 0;
					drawBuffer.nextSubpass(vk::SubpassContents::eInline);
					drawBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicPipelines[1].get());

					// for now just bind it, we can use a different pipeline layout later
					{
						std::array<vk::DescriptorSet, 6> descriptorSets = {
							m_descriptorSet_texture,
							m_descriptorSet_ubo[m_currentframe],
							m_descriptorSet_cameratMat[m_currentframe],
							m_descriptorSet_rendered[renderedInputIdx],
							m_descriptorSet_cameraIndices[m_currentframe],
							m_descriptorSet_portalIndexHelper[m_currentframe],
						};

						drawBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSets, {});
					}

					// draw Portals
					{
						DrawPortalsInfo info = {};
						info.drawBuffer = drawBuffer;
						info.firstCameraIndicesIndex = 1;
						info.iterationElementIndex = iterationElementIndex;
						info.layout = m_pipelineLayout.get();
						info.maxVisiblePortalCount = m_stencilRefTree.GetVisiblePortalCountForLayer(0);
						info.meshDataManager = m_meshData.get();
						info.numBitsToShiftStencil = 0;
						info.stencilRef = 0;


						m_portalManager.DrawPortals(info);
					}
				}
			}

			for (int iteration = 0; iteration < recursionCount; ++iteration)
			{
				const int renderedInputIdx = iteration % 2;
				const bool isLastIteration = (iteration == (recursionCount - 1));

				drawBuffer.nextSubpass(vk::SubpassContents::eInline);

				// clear depth attachment, to be able to render objects "behind" the portal
				{
					vk::ClearAttachment clearDepthOnly = vk::ClearAttachment{}
						.setColorAttachment(1)
						.setAspectMask(vk::ImageAspectFlagBits::eDepth)
						.setClearValue(vk::ClearDepthStencilValue(1.f, 255));


					const vk::ClearRect wholeScreen(vk::Rect2D(vk::Offset2D(0, 0), m_swapchain.extent), 0, 1);
					std::array<vk::ClearAttachment, 1> clearAttachments = { clearDepthOnly, };
					drawBuffer.clearAttachments(clearAttachments, wholeScreen);
				}

				const int drawScenePipelineIdx = (iteration + 1) * 2;
				const int drawPortalPipelineIdx = drawScenePipelineIdx + 1;


				// last iteration draw all portals
				const int numVisiblePortalsforLayer = (iteration == recursionCount - 1)
					? m_portalManager.GetPortalCount()
					: m_stencilRefTree.GetVisiblePortalCountForLayer(iteration + 1);

				const int layerStartIndex = m_stencilRefTree.CalcLayerStartIndex(iteration);
				const int layerEndIndex = layerStartIndex + m_stencilRefTree.CalcLayerElementCount(iteration);

				const uint8_t layerComparMask = m_stencilRefTree.GetLayerCompareMask(iteration);

				//draw Scene
				{
					drawBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicPipelines[drawScenePipelineIdx].get());
					drawBuffer.setStencilCompareMask(vk::StencilFaceFlagBits::eFront, layerComparMask);


					{
						std::array<vk::DescriptorSet, 6> descriptorSets = {
								m_descriptorSet_texture,
								m_descriptorSet_ubo[m_currentframe],
								m_descriptorSet_cameratMat[m_currentframe],
								m_descriptorSet_rendered[renderedInputIdx],
								m_descriptorSet_cameraIndices[m_currentframe],
								m_descriptorSet_portalIndexHelper[m_currentframe],
						};

						drawBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSets, {});
					}

					for (int elementIdx = layerStartIndex; elementIdx < layerEndIndex; ++elementIdx)
					{
						const uint8_t stencilRef = m_stencilRefTree.GetStencilRef(elementIdx);

						drawBuffer.setStencilReference(vk::StencilFaceFlagBits::eFront, stencilRef);
						m_scene->Draw(*m_meshData, m_pipelineLayout.get(), drawBuffer, elementIdx, stencilRef);

						// draw Camera
						{

#if 1
							drawBuffer.bindIndexBuffer(m_meshData->GetIndexBuffer(), 0, MeshDataManager::IndexBufferIndexType);
							vk::DeviceSize vertexBufferOffset = 0;
							drawBuffer.bindVertexBuffers(0, m_meshData->GetVertexBuffer(), vertexBufferOffset);

							const MeshDataRef& cameraMeshRef = m_meshData->GetMeshes()[2];
							const MeshDataRef& cameraMeshRef2 = m_meshData->GetMeshes()[1];

							PushConstant pushConstant = {};
							Transform cameraTransform = camera.m_transform;
							pushConstant.model = cameraTransform.ToMat();
							pushConstant.cameraIdx = elementIdx;

							drawBuffer.pushConstants<PushConstant>(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
							drawBuffer.drawIndexed(cameraMeshRef.indexCount, 1, cameraMeshRef.firstIndex, 0, 1);

							cameraTransform.translation += glm::vec3(0.f, 1.f, 0.f);
							pushConstant.model = cameraTransform.ToMat();
							drawBuffer.pushConstants<PushConstant>(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushConstant);
							drawBuffer.drawIndexed(cameraMeshRef2.indexCount, 1, cameraMeshRef2.firstIndex, 0, 1);

#endif
						}

					}
				}

				// Draw Portals
				{
					const int numRightShifts = m_stencilRefTree.CalcStencilShiftBitsForLayer(iteration + 1);

					const int cameraIndicesLayerStartIndex = isLastIteration ? 0 : m_stencilRefTree.CalcLayerStartIndex(iteration + 1);

					const int firstCameraIndicesOffsetForLayer = isLastIteration ? 0 : m_stencilRefTree.GetVisiblePortalCountForLayer(iteration + 1);

					drawBuffer.nextSubpass(vk::SubpassContents::eInline);
					drawBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicPipelines[drawPortalPipelineIdx].get());
					{
						std::array<vk::DescriptorSet, 6> descriptorSets = {
								m_descriptorSet_texture,
								m_descriptorSet_ubo[m_currentframe],
								m_descriptorSet_cameratMat[m_currentframe],
								m_descriptorSet_rendered[renderedInputIdx],
								m_descriptorSet_cameraIndices[m_currentframe],
								m_descriptorSet_portalIndexHelper[m_currentframe],
						};

						drawBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, descriptorSets, {});
					}

					drawBuffer.setStencilCompareMask(vk::StencilFaceFlagBits::eVkStencilFrontAndBack, layerComparMask);

					for (int elementIdx = layerStartIndex; elementIdx < layerEndIndex; ++elementIdx)
					{

						int childnum = elementIdx - layerStartIndex;
						int firstCameraIndex = cameraIndicesLayerStartIndex + childnum * firstCameraIndicesOffsetForLayer;


						const uint8_t stencilRef = m_stencilRefTree.GetStencilRef(elementIdx);

						// not really needed
						drawBuffer.setStencilReference(vk::StencilFaceFlagBits::eVkStencilFrontAndBack, stencilRef);

						DrawPortalsInfo info = {};
						info.drawBuffer = drawBuffer;
						info.firstCameraIndicesIndex = firstCameraIndex;
						info.iterationElementIndex = elementIdx;
						info.layout = m_pipelineLayout.get();
						info.maxVisiblePortalCount = m_stencilRefTree.GetVisiblePortalCountForLayer(0);
						info.meshDataManager = m_meshData.get();
						info.numBitsToShiftStencil = numRightShifts;
						info.stencilRef = stencilRef;

						m_portalManager.DrawPortals(info);
					}
				}
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
