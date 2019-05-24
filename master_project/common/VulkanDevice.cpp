#include "pch.h"
#include "VulkanDevice.hpp"

using namespace VulkanDevice;

namespace
{
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

	bool HasSurfaceCapabilities(vk::PhysicalDevice device, vk::SurfaceKHR surface)
	{
		VkSurfaceCapabilitiesKHR capabilities =	device.getSurfaceCapabilitiesKHR(surface);
		std::vector<vk::SurfaceFormatKHR> formats = device.getSurfaceFormatsKHR(surface);
		std::vector<vk::PresentModeKHR> presentModes = device.getSurfacePresentModesKHR(surface);

		return !formats.empty() && !presentModes.empty();
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

	constexpr int32_t InvalidQueueScore = std::numeric_limits<int32_t>::max();

	struct CalcQueueScoresStaticData
	{
		std::vector<vk::QueueFamilyProperties> queueProperties;
		std::array<int8_t, 5> flagImportance;
		vk::PhysicalDevice device;
		const vk::SurfaceKHR* optionalSurface;
	};

	// the higher the score, the worse the queue is. 0 would be an optimal queue
// a score of InvalidQueueScore means the queue failed the requirements
// queueFamilyStartIndex indicates how many queues are already taken of that family
	void CalcQueueScores(const CalcQueueScoresStaticData& staticData, const QueueRequirement& requirement, gsl::span<const int32_t> queueFamilyStartIndex, gsl::span<int32_t> outQueueScore)
	{
		std::fill(outQueueScore.begin(), outQueueScore.end(), 0);

		for (size_t i = 0, count = std::size(outQueueScore); i < count; ++i)
		{

			if ((staticData.queueProperties[i].queueFlags & requirement.minFlags) != requirement.minFlags)
			{
				// does not match specification
				outQueueScore[i] = InvalidQueueScore;
				continue;
			}

			// increase score for to many or too few available elements?
			if (static_cast<int32_t>(staticData.queueProperties[i].queueCount) - queueFamilyStartIndex[i] < requirement.mincount)
			{
				// not enough queues available
				outQueueScore[i] = InvalidQueueScore;
				continue;
			}

			if (staticData.optionalSurface != nullptr)
			{
				const bool presentSupport = staticData.device.getSurfaceSupportKHR(i, *staticData.optionalSurface) != VK_FALSE;
				if (presentSupport != requirement.canPresent)
				{
					if (requirement.canPresent)
					{
						// does not match specification
						outQueueScore[i] = InvalidQueueScore;
						continue;
					}
					else
					{
						assert(presentSupport);
						// increase score for unneeded present support
						outQueueScore[i] += 5;
					}
				}
			}

			// increase score for unnecessary flags, even more if many queue would need that flag
			const vk::QueueFlags unneccesaryFlags = staticData.queueProperties[i].queueFlags & (~requirement.minFlags);
			for (vk::QueueFlagBits flagBit : queueFlagBits)
			{
				if (unneccesaryFlags | flagBit)
				{
					const int8_t importance = staticData.flagImportance[QueueFlagBitToInt(flagBit)];
					outQueueScore[i] += importance * importance;
				}
			}
		}
	}

	
}



std::optional<std::vector<QueueResult>> VulkanDevice::IsMatchingDevice(
	const vk::PhysicalDevice& device,
	const DeviceRequirements& requirements,
	const vk::SurfaceKHR* optionalSurface)
{
	if (!HasRequiredExtensions(device, requirements.requiredExtensions))
	{
		return std::nullopt;
	}

	if (optionalSurface && !HasSurfaceCapabilities(device, *optionalSurface))
	{
		return std::nullopt;
	}

	CalcQueueScoresStaticData staticCalcData;
	staticCalcData.flagImportance = FindFlagImportance(requirements.queueRequirements);
	staticCalcData.queueProperties = device.getQueueFamilyProperties();
	staticCalcData.device = device;
	staticCalcData.optionalSurface = optionalSurface;
	const size_t numQueueFamilies = std::size(staticCalcData.queueProperties);

	std::vector<int32_t> queueFamilyStartIndex(numQueueFamilies, 0);
	std::vector<int32_t> queueScore(numQueueFamilies, 0);


	std::vector<QueueResult> results;
	results.reserve(requirements.queueRequirements.size());
	for (const QueueRequirement& queueRequirement : requirements.queueRequirements)
	{
		CalcQueueScores(staticCalcData, queueRequirement, queueFamilyStartIndex, queueScore);
		const auto minElementIt = std::min_element(queueScore.begin(), queueScore.end());
		const size_t bestQueueFamilyIndex = std::distance(queueScore.begin(), minElementIt);
		if (InvalidQueueScore == queueScore[bestQueueFamilyIndex])
		{
			// one queue requirement could not be fulfilled, this device can live up to expectations
			return std::nullopt;
		}

		// currently we only fulfill the minimum requirements and don't tell user whether the queue could do more
		QueueResult queueResult;
		queueResult.canPresent = queueRequirement.canPresent;
		queueResult.count = queueRequirement.mincount;
		queueResult.familyIndex = gsl::narrow<uint32_t>(bestQueueFamilyIndex);
		queueResult.flags = queueRequirement.minFlags;
		queueResult.offset = queueFamilyStartIndex[bestQueueFamilyIndex];

		// queue family now has less available queues
		queueFamilyStartIndex[bestQueueFamilyIndex] += queueRequirement.mincount;

		results.push_back(queueResult);
	}

	assert(results.size() == requirements.queueRequirements.size());
	return std::optional<std::vector<QueueResult>>(std::move(results));
}

std::optional<VulkanDevice::PickDeviceResult> VulkanDevice::PickPhysicalDevice(vk::Instance instance, const DeviceRequirements& requirements, const vk::SurfaceKHR* optionalSurface)
{
	std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
	for (const vk::PhysicalDevice& device : physicalDevices)
	{
		std::optional<std::vector<QueueResult>> maybeResult = IsMatchingDevice(device, requirements, optionalSurface);
		if (maybeResult)
		{
			return std::optional<PickDeviceResult>(std::in_place, device, std::move(*maybeResult));
		}
	}
	return std::nullopt;
}

vk::UniqueDevice VulkanDevice::CreateLogicalDevice(vk::PhysicalDevice device, gsl::span<const QueueResult> queues, gsl::span<const char*> enabledValidationLayers, gsl::span<const char*> enabledExtensions)
{
	// used everywhere as priorities are mandatory, be in truth we don't care about them (at the moment)
	// works as long as we don't create more than std::size(priorities) queues of the same family
	std::array<float, 32> priorities;
	priorities.fill(1.f);

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (const QueueResult& queue : queues)
	{
		const int32_t familyIndex = queue.familyIndex;
		const auto fountIt = std::find_if(queueCreateInfos.begin(), queueCreateInfos.end(), [familyIndex](const vk::DeviceQueueCreateInfo & info)
			{
				return info.queueFamilyIndex == familyIndex;
			});

		if (queueCreateInfos.end() == fountIt)
		{
			vk::DeviceQueueCreateInfo newInfo(vk::DeviceQueueCreateFlags(), familyIndex, queue.count, std::data(priorities));
			assert(newInfo.queueCount < std::size(priorities));
			queueCreateInfos.push_back(newInfo);
		}
		else
		{
			fountIt->queueCount += queue.count;
			assert(fountIt->queueCount < std::size(priorities));
		}
	}

	vk::PhysicalDeviceFeatures deviceFeatures{};

	vk::DeviceCreateInfo deviceCreateInfo(
		vk::DeviceCreateFlags(),
		gsl::narrow<uint32_t>(std::size(queueCreateInfos)), std::data(queueCreateInfos),
		gsl::narrow<uint32_t>(std::size(enabledValidationLayers)), std::data(enabledValidationLayers),
		gsl::narrow<uint32_t>(std::size(enabledExtensions)), std::data(enabledExtensions),
		&deviceFeatures);


	return device.createDeviceUnique(deviceCreateInfo);
}

//bool VulkanDevice::IsDeviceSuitable(vk::PhysicalDevice device, gsl::span<const char*> requiredExtensions)
//{
//	vk::PhysicalDeviceProperties properties = device.getProperties();
//	vk::PhysicalDeviceFeatures features = device.getFeatures();
//
//	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = device.getQueueFamilyProperties();
//
//	const bool hasRequiredQueueFamily = std::any_of(queueFamilyProperties.cbegin(), queueFamilyProperties.cend(), [&device](const vk::QueueFamilyProperties & queueFamilyProperty)
//		{
//			return queueFamilyProperty.queueCount > 0
//				&& queueFamilyProperty.queueFlags& vk::QueueFlagBits::eGraphics
//				;
//		});
//
//
//	return true
//		&& HasRequiredExtensions(device, requiredExtensions)
//		&& hasRequiredQueueFamily
//		//&& vk::PhysicalDeviceType::eDiscreteGpu == properties.deviceType
//		//&& features.geometryShader
//		;
//}
