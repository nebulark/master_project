#include "pch.hpp"
#include "DebugUtils.hpp"

namespace
{
	VkBool32 DebugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
	{
		vk::DebugUtilsMessageSeverityFlagBitsEXT severity = static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity);
		vk::DebugUtilsMessageTypeFlagsEXT types(messageTypes);

		std::cerr << vk::to_string(severity) << ' ' << vk::to_string(types) << ' ' << pCallbackData->pMessage << '\n';

		constexpr VkBool32 shouldAbortFunctionCall = VK_TRUE;
		if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
		{
			throw std::runtime_error(pCallbackData->pMessage);
		}
		return shouldAbortFunctionCall;
	}

}

vk::UniqueDebugUtilsMessengerEXT DebugUtils::CreateDebugUtilsMessenger(vk::Instance instance)
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
