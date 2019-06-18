#pragma once
#include <vulkan/vulkan.hpp>
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include <map>
#include "VmaTraits.hpp"

template<typename Type>
class VmaAllocationsPool
{
public:

	using Traits = VmaTraits<Type>;
	
	void Init(VmaAllocator allocator);


	Type Alloc(const typename Traits::CreateInfo& createInfo, const VmaAllocationCreateInfo& vmaAllocInfo);

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
	const typename Traits::CreateInfo& createInfo,
	const VmaAllocationCreateInfo& vmaAllocInfo)
{
	assert(m_allocator);

	Type outResource;
	VmaAllocation outallocation;
	vk::Result result = Traits::Create(m_allocator, createInfo, vmaAllocInfo, outResource, outallocation, nullptr);

	if (result != vk::Result::eSuccess)
	{
		throw std::runtime_error(vk::to_string(result));
	}

	const int index = m_counter;
	++m_counter;
	m_storage.insert(std::make_pair(outResource, outallocation));

	return outResource;
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
		Traits::Destroy(m_allocator, it->first, it->second);
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
		Traits::Destroy(m_allocator, e.first, e.second);
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

