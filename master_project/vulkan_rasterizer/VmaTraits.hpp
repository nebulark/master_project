#pragma once
#include <vulkan/vulkan.hpp>
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

// Traits for vk::Buffer and vk::Image to determine the right functions for Vma

template<typename Type>
struct VmaTraits_Impl;

template<>
struct VmaTraits_Impl<vk::Buffer>
{
	using Type = vk::Buffer;
	using CType = VkBuffer;

	using CreateInfo = vk::BufferCreateInfo;
	using CCreateInfo = VkBufferCreateInfo;

	static VkResult Create(VmaAllocator allocator,
		const VkBufferCreateInfo* pBufferCreateInfo,
		const VmaAllocationCreateInfo* pAllocationCreateInfo,
		VkBuffer* pBuffer,
		VmaAllocation* pAllocation,
		VmaAllocationInfo* pAllocationInfo)
	{
		return vmaCreateBuffer(allocator, pBufferCreateInfo, pAllocationCreateInfo, pBuffer, pAllocation, pAllocationInfo);
	}

	static void Destroy(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
	{
		vmaDestroyBuffer(allocator, buffer, allocation);
	}
};

template<>
struct VmaTraits_Impl<vk::Image>
{
	using Type = vk::Image;
	using CType = VkImage;

	using CreateInfo = vk::ImageCreateInfo;
	using CCreateInfo = VkImageCreateInfo;

	static VkResult Create(VmaAllocator allocator,
		const CCreateInfo* imageCreateInfo,
		const VmaAllocationCreateInfo* pAllocationCreateInfo,
		CType* pImage,
		VmaAllocation* pAllocation,
		VmaAllocationInfo* pAllocationInfo)
	{
		return vmaCreateImage(allocator, imageCreateInfo, pAllocationCreateInfo, pImage, pAllocation, pAllocationInfo);
	}

	static void Destroy(VmaAllocator allocator, CType image, VmaAllocation allocation)
	{
		vmaDestroyImage(allocator, image, allocation);
	}
};


// convenience trait for vulkan hpp types
template<typename Type>
struct VmaTraits
{
	using Impl = VmaTraits_Impl<Type>;

	using CreateInfo = typename Impl::CreateInfo;

	static vk::Result Create(VmaAllocator allocator,
		const CreateInfo& createInfo,
		const VmaAllocationCreateInfo& allocationCreateInfo,
		Type& outObject,
		VmaAllocation& outAllocation,
		VmaAllocationInfo* outOptionalAllocationInfo)
	{
		const Impl::CCreateInfo& cCreateInfo = createInfo;
		typename Impl::CType cObject;

		VkResult result = Impl::Create(allocator, &cCreateInfo, &allocationCreateInfo, &cObject, &outAllocation, outOptionalAllocationInfo);
		outObject = cObject;
		return vk::Result(result);
	}

	static void Destroy(VmaAllocator allocator, Type object, VmaAllocation allocation)
	{
		Impl::Destroy(allocator, object, allocation);
	}

};

