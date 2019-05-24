// raytrace_software.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

#include "common/Deleters.hpp"
#include "common/VulkanDebug.hpp"
#include "common/VulkanDevice.hpp"
#include "common/VulkanUtils.hpp"


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



int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	{
		constexpr int width = 1920;
		constexpr int height = 1080;


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

		const vk::SurfaceFormatKHR surfaceFormat = VulkanUtils::ChooseSurfaceFormat(
			deviceResult.device.getSurfaceFormatsKHR(surface.get()),
			preferedFormats);

		const vk::PresentModeKHR preferedPresentationModes[] = { vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate };

		const vk::PresentModeKHR presentMode = VulkanUtils::ChoosePresentMode(
			deviceResult.device.getSurfacePresentModesKHR(surface.get()),
			preferedPresentationModes
		);

		const vk::SurfaceCapabilitiesKHR capabilities = deviceResult.device.getSurfaceCapabilitiesKHR(surface.get());
		const vk::Extent2D extent = VulkanUtils::ChooseExtent(
			capabilities,
			vk::Extent2D(width, height)
		);

		const uint32_t imageCount = VulkanUtils::ChooseImageCount(capabilities, capabilities.minImageCount + 1);

		vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR();
		swapchainCreateInfo
			.setSurface(surface.get())
			.setMinImageCount(imageCount)
			.setImageFormat(surfaceFormat.format)
			.setImageColorSpace(surfaceFormat.colorSpace)
			.setImageExtent(extent)
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

		

		//VulkanDevice::LogicalDevice logicalDevice = VulkanDevice::(instance.get(), enabledValidationLayers, deviceExtensions, surface.get());
		//logicalDevice.device->getQueue(logicalDevice.graphicsPresentQueueIdx, 0);

	}
	SDL_Quit();
		

	return 0;
	
}


