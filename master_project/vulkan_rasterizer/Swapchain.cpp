#include "pch.hpp"
#include "Swapchain.hpp"
#include "common/VulkanUtils.hpp"


namespace
{

	constexpr vk::SurfaceFormatKHR preferedSurfaceFormats[] = {
			vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear},
	};

	const vk::PresentModeKHR preferedPresentationModes[] = { vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate };

}


Swapchain Swapchain::Create(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface, vk::Extent2D extent)
{
	Swapchain swapchain;
	std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
	std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
	vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);



	swapchain.surfaceFormat = VulkanUtils::ChooseSurfaceFormat(
		formats, preferedSurfaceFormats);


	swapchain.presentMode = VulkanUtils::ChoosePresentMode(
		presentModes, preferedPresentationModes);


	assert(capabilities.currentExtent.width >= 0 && capabilities.currentExtent.height >= 0);

	swapchain.extent = VulkanUtils::ChooseExtent(capabilities, extent);

	const uint32_t imageCount = VulkanUtils::ChooseImageCount(capabilities, capabilities.minImageCount + 1);

	const vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
		.setSurface(surface)
		.setMinImageCount(imageCount)
		.setImageFormat(swapchain.surfaceFormat.format)
		.setImageColorSpace(swapchain.surfaceFormat.colorSpace)
		.setImageExtent(swapchain.extent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setImageSharingMode(vk::SharingMode::eExclusive) // we only have one queue
		.setPreTransform(capabilities.currentTransform)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(swapchain.presentMode)
		.setClipped(true)
		.setOldSwapchain(vk::SwapchainKHR());

	swapchain.swapchain = device.createSwapchainKHRUnique(swapchainCreateInfo);

	std::vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR(swapchain.swapchain.get());
	const vk::ImageViewCreateInfo imageViewCreateInfoPrototype = vk::ImageViewCreateInfo()
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(swapchain.surfaceFormat.format)
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

	swapchain.imageViews.reserve(swapchainImages.size());
	for (vk::Image image : swapchainImages)
	{
		const vk::ImageViewCreateInfo imageViewCreateInfo = vk::ImageViewCreateInfo(imageViewCreateInfoPrototype)
			.setImage(image);
		swapchain.imageViews.push_back(device.createImageViewUnique(imageViewCreateInfo));
	}

	return swapchain;
}
