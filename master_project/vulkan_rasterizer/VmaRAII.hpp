#pragma once
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include <memory>
#include <vulkan/vulkan.hpp>

namespace DetailVmaRAII
{

}


namespace vk
{

  template <typename Dispatch> class UniqueHandleTraits<VmaAllocator, Dispatch>
  {
  public: using deleter =
	  struct {
	  void destroy(VmaAllocator allocator)
	  {
		  vmaDestroyAllocator(allocator);
	  }
  };
  };
/*
#define DEFINE_VK_DELETER_TRAIT(handletype, deleterfunctionname) \ 
template <typename Dispatch> class UniqueHandleTraits<handletype, Dispatch> \
  {\
  public: using deleter =\
	  struct {
	  void destroy(handletype handle)\
	  {\
		  deleterfunctionname(handle);\
	  }\
  };\
  };


 #undef VK_DELETER_TRAIT
 */
}
namespace VmaRAII
{
	using UniqueVmaAllocator = vk::UniqueHandle<VmaAllocator, vk::DispatchLoaderStatic>;
	inline UniqueVmaAllocator CreateVmaAllocatorUnique(const VmaAllocatorCreateInfo& createInfo)
	{
		VmaAllocator alloc;
		VkResult vkResult = vmaCreateAllocator(&createInfo, &alloc);
		if (vkResult != VkResult::VK_SUCCESS)
		{
			vk::Result result = reinterpret_cast<vk::Result&>(vkResult);
			throw std::runtime_error(vk::to_string(result));
		}
		return UniqueVmaAllocator(alloc);
	}

}

