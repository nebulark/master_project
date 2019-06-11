#pragma once
#include <vulkan/vulkan.hpp>
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include <map>

template<typename Type>
struct VmaAllocationElement
{
	Type resource;
	VmaAllocation allocation;
};

template<typename Type>
struct VmaAllocationPoolTrait;

template<>
struct VmaAllocationPoolTrait<vk::Buffer>
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
struct VmaAllocationPoolTrait<vk::Image>
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


template<typename Type>
class VmaAllocationsPool
{
public:

	using Trait = VmaAllocationPoolTrait<Type>;
	using Element = VmaAllocationElement<Type>;
	
	void Init(VmaAllocator allocator);


	Type Alloc(const typename Trait::CreateInfo& createInfo, const VmaAllocationCreateInfo& vmaAllocInfo);

	void Destroy(Type resource);

	VmaAllocation* GetAllocation(Type resource);
	const VmaAllocation* GetAllocation(Type resource) const;

	VmaAllocationsPool() = default;
	VmaAllocationsPool(const VmaAllocationsPool& rhs) = delete;
	VmaAllocationsPool& operator=(const VmaAllocationsPool& rhs) = delete;
	
	void swap(VmaAllocationsPool& rhs);

	VmaAllocationsPool(VmaAllocationsPool&& rhs) = default;
	VmaAllocationsPool& operator=(VmaAllocationsPool&& rhs) = default;
	~VmaAllocationsPool();

	VmaAllocator GetVmaAllocator() { return m_allocator; }
	
private:

	// TODO: Improve when performance is needed
	int m_counter;
	VmaAllocator m_allocator;
	std::map<Type, VmaAllocation> m_storage;
};

template<typename Type>
void VmaAllocationsPool<Type>::Init(VmaAllocator allocator)
{
	m_allocator = allocator;
	m_counter = 0;
}

template<typename Type>
Type VmaAllocationsPool<Type>::Alloc(
	const typename Trait::CreateInfo& createInfo,
	const VmaAllocationCreateInfo& vmaAllocInfo)
{
	assert(m_allocator);

	typename Trait::CType cresource;
	VmaAllocation allocation;

	VkResult result = Trait::Create(
		m_allocator,
		reinterpret_cast<const Trait::CCreateInfo*>(&createInfo),
		&vmaAllocInfo,
		&cresource,
		&allocation,
		nullptr);

	if (result != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error(vk::to_string(vk::Result(result)));
	}

	Type resource(cresource);
	const int index = m_counter;
	++m_counter;
	m_storage.insert(std::make_pair(resource, allocation));

	return resource;
}

template<typename Type>
void VmaAllocationsPool<Type>::Destroy(Type resource)
{
	auto it = m_storage.find(resource);
	if (m_storage.end() == it)
	{
		assert(false && "Trying to destroy element twice");
	}
	else
	{
		Trait::Destroy(m_allocator, it->first, it->second);
		m_storage.erase(it);
	}
}

template<typename Type>
VmaAllocation* VmaAllocationsPool<Type>::GetAllocation(Type resource)
{
	auto it = m_storage.find(resource);
	if (m_storage.end() == it)
	{
		return nullptr;
	}
	else
	{
		return &(it->second);
	}
}

template<typename Type>
const VmaAllocation* VmaAllocationsPool<Type>::GetAllocation(Type resource) const
{
	auto it = m_storage.find(resource);
	if (m_storage.end() == it)
	{
		return nullptr;
	}
	else
	{
		return &(it->second);
	}
}


template<typename Type>
VmaAllocationsPool<Type>::~VmaAllocationsPool()
{
	for (std::pair<const Type, VmaAllocation>& e : m_storage)
	{
		Trait::Destroy(m_allocator, e.first, e.second);
	}
}

template<typename Type>
void VmaAllocationsPool<Type>::swap(VmaAllocationsPool& rhs)
{
	std::swap(m_storage, rhs.m_storage);
	std::swap(m_counter, rhs.m_counter);
	std::swap(m_allocator, rhs.m_allocator);
}

using VmaImagePool = VmaAllocationsPool<vk::Image>;
using VmaBufferPool = VmaAllocationsPool<vk::Buffer>;

