#pragma once
#include <vulkan/vulkan.hpp>

namespace VulkanDebug
{
	void LoadDebugUtilsExtension(const vk::Instance& instance);

	template<typename T>
	constexpr vk::ObjectType GetObjectType()
	{
      if constexpr(std::is_same_v<T, vk::Instance>){return vk::ObjectType::eInstance;}
      else if constexpr(std::is_same_v<T, vk::PhysicalDevice>){return vk::ObjectType::ePhysicalDevice;}
      else if constexpr(std::is_same_v<T, vk::Device>){return vk::ObjectType::eDevice;}
      else if constexpr(std::is_same_v<T, vk::Queue>){return vk::ObjectType::eQueue;}
      else if constexpr(std::is_same_v<T, vk::Semaphore>){return vk::ObjectType::eSemaphore;}
      else if constexpr(std::is_same_v<T, vk::CommandBuffer>){return vk::ObjectType::eCommandBuffer;}
      else if constexpr(std::is_same_v<T, vk::Fence>){return vk::ObjectType::eFence;}
      else if constexpr(std::is_same_v<T, vk::DeviceMemory>){return vk::ObjectType::eDeviceMemory;}
      else if constexpr(std::is_same_v<T, vk::Buffer>){return vk::ObjectType::eBuffer;}
      else if constexpr(std::is_same_v<T, vk::Image>){return vk::ObjectType::eImage;}
      else if constexpr(std::is_same_v<T, vk::Event>){return vk::ObjectType::eEvent;}
      else if constexpr(std::is_same_v<T, vk::QueryPool>){return vk::ObjectType::eQueryPool;}
      else if constexpr(std::is_same_v<T, vk::BufferView>){return vk::ObjectType::eBufferView;}
      else if constexpr(std::is_same_v<T, vk::ImageView>){return vk::ObjectType::eImageView;}
      else if constexpr(std::is_same_v<T, vk::ShaderModule>){return vk::ObjectType::eShaderModule;}
      else if constexpr(std::is_same_v<T, vk::PipelineCache>){return vk::ObjectType::ePipelineCache;}
      else if constexpr(std::is_same_v<T, vk::PipelineLayout>){return vk::ObjectType::ePipelineLayout;}
      else if constexpr(std::is_same_v<T, vk::RenderPass>){return vk::ObjectType::eRenderPass;}
      else if constexpr(std::is_same_v<T, vk::Pipeline>){return vk::ObjectType::ePipeline;}
      else if constexpr(std::is_same_v<T, vk::DescriptorSetLayout>){return vk::ObjectType::eDescriptorSetLayout;}
      else if constexpr(std::is_same_v<T, vk::Sampler>){return vk::ObjectType::eSampler;}
      else if constexpr(std::is_same_v<T, vk::DescriptorPool>){return vk::ObjectType::eDescriptorPool;}
      else if constexpr(std::is_same_v<T, vk::DescriptorSet>){return vk::ObjectType::eDescriptorSet;}
      else if constexpr(std::is_same_v<T, vk::Framebuffer>){return vk::ObjectType::eFramebuffer;}
      else if constexpr(std::is_same_v<T, vk::CommandPool>){return vk::ObjectType::eCommandPool;}
      else if constexpr(std::is_same_v<T, vk::SamplerYcbcrConversion>){return vk::ObjectType::eSamplerYcbcrConversion;}
      else if constexpr(std::is_same_v<T, vk::DescriptorUpdateTemplate>){return vk::ObjectType::eDescriptorUpdateTemplate;}
      else if constexpr(std::is_same_v<T, vk::SurfaceKHR>){return vk::ObjectType::eSurfaceKHR;}
      else if constexpr(std::is_same_v<T, vk::SwapchainKHR>){return vk::ObjectType::eSwapchainKHR;}
      else if constexpr(std::is_same_v<T, vk::DisplayKHR>){return vk::ObjectType::eDisplayKHR;}
      else if constexpr(std::is_same_v<T, vk::DisplayModeKHR>){return vk::ObjectType::eDisplayModeKHR;}
      else if constexpr(std::is_same_v<T, vk::DebugReportCallbackEXT>){return vk::ObjectType::eDebugReportCallbackEXT;}
      else if constexpr(std::is_same_v<T, vk::ObjectTableNVX>){return vk::ObjectType::eObjectTableNVX;}
      else if constexpr(std::is_same_v<T, vk::IndirectCommandsLayoutNVX>){return vk::ObjectType::eIndirectCommandsLayoutNVX;}
      else if constexpr(std::is_same_v<T, vk::DebugUtilsMessengerEXT>){return vk::ObjectType::eDebugUtilsMessengerEXT;}
      else if constexpr(std::is_same_v<T, vk::ValidationCacheEXT>){return vk::ObjectType::eValidationCacheEXT;}
      else if constexpr(std::is_same_v<T, vk::AccelerationStructureNV>){return vk::ObjectType::eAccelerationStructureNV;}
	  else { static_assert(false, "Invalid Object Type"); return vk::ObjectType::eUnknown; }
	}



	template<typename T>
	void SetObjectName(vk::Device device, T Object, const char* name)
	{
		uint64_t handle = 0;
		if constexpr(sizeof(Object) == sizeof(uint64_t))
		{
			handle = reinterpret_cast<uint64_t&>(Object);
		}
		else if constexpr(sizeof(Object) == sizeof(uint32_t))
		{
			handle = reinterpret_cast<uint32_t&>(Object);
		}
		else
		{
			static_assert(false, "object Size not supported");
		}

		vk::ObjectType objectType = GetObjectType<T>();

		device.setDebugUtilsObjectNameEXT(
			vk::DebugUtilsObjectNameInfoEXT{}
			.setObjectHandle(handle)
			.setObjectType(objectType)
			.setPObjectName(name));
	}

	// new Extension

}