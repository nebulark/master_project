#pragma once
#include <vulkan/vulkan.hpp>
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include "VmaTraits.hpp"

template<typename Type>
class UniqueVmaObject
{
public:
	using Traits = VmaTraits<Type>;

	UniqueVmaObject() : m_object(nullptr), m_allocator(nullptr), m_allocation(nullptr){}

	UniqueVmaObject(
		VmaAllocator allocator, const typename Traits::CreateInfo& createInfo,
		const VmaAllocationCreateInfo& vmaAllocationCreateInfo);

	UniqueVmaObject(const UniqueVmaObject&) = delete;
	UniqueVmaObject& operator=(const UniqueVmaObject&) = delete;

	UniqueVmaObject(UniqueVmaObject&& rhs) noexcept;
	UniqueVmaObject& operator=(UniqueVmaObject&& rhs) noexcept;

	~UniqueVmaObject();

	Type Get() { return m_object; }
	VmaAllocator GetAllocator() { return m_allocator; }
	VmaAllocation GetAllocation() { return m_allocation; }
private:

	Type m_object;
	VmaAllocator m_allocator;
	VmaAllocation m_allocation;
};

template<typename Type>
inline UniqueVmaObject<Type>::UniqueVmaObject(VmaAllocator allocator, const typename Traits::CreateInfo& createInfo, const VmaAllocationCreateInfo& vmaAllocationCreateInfo)
	: m_object(nullptr)
	, m_allocator(allocator)
	, m_allocation(nullptr)
{
	vk::Result result =	Traits::Create(allocator, createInfo, vmaAllocationCreateInfo, m_object, m_allocation, nullptr);
}

template<typename Type>
inline UniqueVmaObject<Type>::UniqueVmaObject(UniqueVmaObject<Type>&& rhs) noexcept
	: m_object(rhs.m_object)
	, m_allocator(rhs.m_allocator)
	, m_allocation(rhs.m_allocation)
{
	rhs.m_object = nullptr;
}

template<typename Type>
inline UniqueVmaObject<Type>& UniqueVmaObject<Type>::operator=(UniqueVmaObject<Type>&& rhs) noexcept
{
	std::swap(m_object, rhs.m_object);
	std::swap(m_allocator, rhs.m_allocator);
	std::swap(m_allocation, rhs.m_allocation);
	return *this;
}

template<typename Type>
inline UniqueVmaObject<Type>::~UniqueVmaObject()
{
	if (m_object)
	{
		Traits::Destroy(m_allocator, m_object, m_allocation);
	}
}

using UniqueVmaBuffer = UniqueVmaObject<vk::Buffer>;
using UniqueVmaImage = UniqueVmaObject<vk::Image>;


