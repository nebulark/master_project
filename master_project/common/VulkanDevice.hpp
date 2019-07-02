#pragma once
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <gsl/gsl>

namespace VulkanDevice
{
	struct QueueRequirement
	{
		vk::QueueFlags minFlags;
		int32_t mincount;
		bool canPresent;
	};

	struct DeviceRequirements
	{
		gsl::span<const char*> requiredExtensions;
		vk::PhysicalDeviceFeatures requiredFeatures;
		gsl::span<QueueRequirement> queueRequirements;
	};

	struct QueueResult
	{	
		uint32_t familyIndex;
		uint32_t count;
		uint32_t offset;

		vk::QueueFlags flags;
		bool canPresent;
	};

	struct PickDeviceResult
	{
		vk::PhysicalDevice device;
		std::vector<QueueResult> queueResult;
		PickDeviceResult() = default;
		PickDeviceResult(vk::PhysicalDevice device,	std::vector<QueueResult>&& queueResult)
			: device(device), queueResult(std::move(queueResult)){}

	};	
	
	// checks whether the device fulfills requirements and returns queue results if successful
	std::optional<std::vector<QueueResult>> IsMatchingDevice(
		const vk::PhysicalDevice& device,
		const DeviceRequirements& requirements,
		const vk::SurfaceKHR* optionalSurface);

	// uses IsMatchingDevice on all available devices to find the first that matches requirements
	std::optional<PickDeviceResult> PickPhysicalDevice(
		vk::Instance instance,
		const DeviceRequirements& requirements,
		const vk::SurfaceKHR* optionalSurface);

	// creates device using the queue results from previous functions
	vk::UniqueDevice CreateLogicalDevice(
		vk::PhysicalDevice device,
		gsl::span<const QueueResult> queues,
		gsl::span<const char*> enabledValidationLayers,
		gsl::span<const char*> enabledExtensions, const vk::PhysicalDeviceFeatures& deviceFeatures);
}
