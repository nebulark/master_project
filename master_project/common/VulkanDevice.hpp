#pragma once
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <array>

namespace VulkanDevice
{
	struct LogicalDevice
	{
		vk::UniqueHandle<vk::Device, vk::DispatchLoaderStatic> device;
		uint32_t graphicsPresentQueueIdx;
		uint32_t graphicsPresentQueueCount;
	};

	struct QueueRequirement
	{
		vk::QueueFlags minFlags;
		int32_t mincount;
		bool canPresent;
	};

	struct DeviceRequirements
	{
		gsl::span<const char*> requiredExtensions;
		gsl::span<QueueRequirement> queueRequirements;
	};

	struct QueueResult
	{	
		uint32_t familyIndex;
		int32_t count;
		int32_t offset;

		vk::QueueFlags flags;
		bool canPresent;
	};

	constexpr int32_t QueueFlagBitToInt(vk::QueueFlagBits FlagBit)
	{
		switch (FlagBit)
		{
		case vk::QueueFlagBits::eGraphics:
			return 0;
		case vk::QueueFlagBits::eCompute:
			return 1;
		case vk::QueueFlagBits::eTransfer:
			return 2;
		case vk::QueueFlagBits::eSparseBinding:
			return 3;
		case vk::QueueFlagBits::eProtected:
			return 4;
		default:
			assert(false);
			return 0;
		}
	}

	constexpr std::array<vk::QueueFlagBits, 5> queueFlagBits = {
			vk::QueueFlagBits::eGraphics,
			vk::QueueFlagBits::eCompute,
			vk::QueueFlagBits::eTransfer,
			vk::QueueFlagBits::eSparseBinding,
			vk::QueueFlagBits::eProtected,
	};

	bool HasRequiredExtensions(vk::PhysicalDevice device, gsl::span<const char*> requiredExtensions)
	{
		std::vector<vk::ExtensionProperties> extensionProperties = device.enumerateDeviceExtensionProperties();
		return std::all_of(requiredExtensions.cbegin(), requiredExtensions.cend(), [&extensionProperties](const char* requiredExtension)
			{
				return std::any_of(extensionProperties.cbegin(), extensionProperties.cend(), [requiredExtension](const vk::ExtensionProperties & availableExtensionProperty)
					{
						return std::strcmp(availableExtensionProperty.extensionName, requiredExtension) == 0;
					});
			});
	}

	std::array<int8_t, 5> FindFlagImportance(gsl::span<QueueRequirement> queueRequirements)
	{
		std::array<int8_t, 5> importance;
		importance.fill(0);

		for (QueueRequirement requirment : queueRequirements)
		{
			for (vk::QueueFlagBits flagBit : queueFlagBits)
			{
				if (requirment.minFlags | flagBit)
				{
					++importance[QueueFlagBitToInt(flagBit)];
				}
			}
		}

		return importance;
	}

	constexpr int32_t InvalidQueueScore = -1;

	struct CalcQueueScoresStaticData
	{
		gsl::span<const vk::QueueFamilyProperties> queueProperties;
		gsl::span<int8_t, 5> flagImportance;
		vk::PhysicalDevice device;
		vk::SurfaceKHR surface;
	};

	void CalcQueueScores(
		const CalcQueueScoresStaticData& staticData,
		const QueueRequirement& requirement,
		gsl::span<const int32_t> queueStartIndex,
		gsl::span<int32_t> outQueueScore)
	{
		for (size_t i = 0, count = std::size(outQueueScore); i < count; ++i)
		{
			if ((staticData.queueProperties[i].queueFlags & requirement.minFlags) != requirement.minFlags)
			{
				// does not match specification
				outQueueScore[i] = InvalidQueueScore;
				continue;
			}

			if (staticData.queueProperties[i].queueCount - queueStartIndex[i] < requirement.mincount)
			{
				// not enough queues available
				outQueueScore[i] = InvalidQueueScore;
				continue;
			}

			vk::Bool32 presentSupport = staticData.device.getSurfaceSupportKHR(i, staticData.surface);
			if (requirement.canPresent)
			{
				if (!presentSupport)
				{
					// does not match specification
					outQueueScore[i] = InvalidQueueScore;
					continue;
				}
			}
			// !requirement.canPresent
			else
			{
				if (presentSupport)
				{
					// increase score for unneeded present support
					outQueueScore[i] += 100;
				}
			}
		

			const vk::QueueFlags unneccesaryFlags = staticData.queueProperties[i].queueFlags & (~requirement.minFlags);

			

		}
	}

	std::optional<uint32_t> IsMatchingDevice(const vk::PhysicalDevice& device, const DeviceRequirements& requirements)
	{
		if (!HasRequiredExtensions(device, requirements.requiredExtensions))
		{
			return std::nullopt;
		}

		std::array<int8_t, 5> flagImportance = FindFlagImportance(requirements.queueRequirements);
		
		const std::vector<vk::QueueFamilyProperties> queueFamilyProperties = device.getQueueFamilyProperties();
		std::vector<int32_t> queueStartIndex(queueFamilyProperties.size(),0);
		std::vector<int32_t> queueScore(queueFamilyProperties.size(), 0);


		std::vector<QueueResult> Result;
		Result.reserve(queueFamilyProperties.size());
		for (const QueueRequirement& queueRequirement : requirements.queueRequirements)
		{
			for (size_t queueIndex = 0, queueCount = std::size(queueFamilyProperties); queueIndex < queueCount; ++queueIndex)
			{
				if ()
			}
		}
	}

	std::optional<vk::PhysicalDevice> PickPhysicalDevice(vk::Instance instance, const DeviceRequirements& requirements)
	{
		std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
		//for(size_t i = 0, count = std::size(physicalDevices); i < count; ++i)
		for (const vk::PhysicalDevice& device : physicalDevices)
		{
			IsMatchingDevice(device, requirements);


			
		}
	}


	bool IsDeviceSuitable(vk::PhysicalDevice device, gsl::span<const char*> requiredExtensions)
	{
		vk::PhysicalDeviceProperties properties = device.getProperties();
		vk::PhysicalDeviceFeatures features = device.getFeatures();

		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = device.getQueueFamilyProperties();

		const bool hasRequiredQueueFamily = std::any_of(queueFamilyProperties.cbegin(), queueFamilyProperties.cend(), [&device](const vk::QueueFamilyProperties & queueFamilyProperty)
			{
				return queueFamilyProperty.queueCount > 0
					&& queueFamilyProperty.queueFlags& vk::QueueFlagBits::eGraphics
					;
			});


		return true
			&& HasRequiredExtensions(device, requiredExtensions)
			&& hasRequiredQueueFamily
			//&& vk::PhysicalDeviceType::eDiscreteGpu == properties.deviceType
			//&& features.geometryShader
			;
	}


	std::optional<vk::PhysicalDevice> PickPhysicalDevice2(vk::Instance instance, gsl::span<const char*> enabledExtensions)
	{
		std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
		auto suitableDeviceIt = std::find_if(physicalDevices.begin(), physicalDevices.end(), [enabledExtensions](vk::PhysicalDevice device)
			{
				return IsDeviceSuitable(device, enabledExtensions);
			});

		if (suitableDeviceIt == physicalDevices.end())
		{
			return std::nullopt;
		}
		else
		{
			return *suitableDeviceIt;
		}
	}



	LogicalDevice CreateLogicalDevice(
		vk::Instance instance, gsl::span<const char*> enabledValidationLayers,
		gsl::span<const char*> enabledExtensions, vk::SurfaceKHR surface)
	{
		std::optional<vk::PhysicalDevice> maybe_device = PickPhysicalDevice(instance, enabledExtensions);
		assert(maybe_device);
		vk::PhysicalDevice device = *maybe_device;

		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = device.getQueueFamilyProperties();

		constexpr uint32_t invalidQueueIdx = -1;
		uint32_t graphicsPresentQueueIdx = invalidQueueIdx;
		for (uint32_t i = 0, count = gsl::narrow<uint32_t>(std::size(queueFamilyProperties)); i < count; ++i)
		{
			vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, surface);
			vk::SurfaceCapabilitiesKHR capabilites = device.getSurfaceCapabilitiesKHR(surface);

			const bool IsValid = queueFamilyProperties[i].queueCount > 0
				&& queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics
				&& presentSupport;

			if (IsValid)
			{
				graphicsPresentQueueIdx = i;
				break;
			}
		}

		assert(graphicsPresentQueueIdx != invalidQueueIdx);

		constexpr uint32_t graphicsQueueCount = 1;

		const float priorities[graphicsQueueCount] = { 1.f };
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), graphicsPresentQueueIdx, graphicsQueueCount, priorities);

		vk::PhysicalDeviceFeatures deviceFeatures{};

		vk::DeviceCreateInfo deviceCreateInfo(
			vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo,
			gsl::narrow<uint32_t>(std::size(enabledValidationLayers)), std::data(enabledValidationLayers),
			0, nullptr,
			&deviceFeatures);


		return LogicalDevice{
			device.createDeviceUnique(deviceCreateInfo),
			graphicsPresentQueueIdx,
			graphicsQueueCount
		};
	}
}