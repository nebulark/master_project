#pragma once
#include <vulkan/vulkan.hpp>
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

class UniqueVmaBuffer
{
public:
	UniqueVmaBuffer() : m_buffer(nullptr), m_allocator(nullptr), m_allocation(nullptr){}

	UniqueVmaBuffer(
		VmaAllocator allocator, const vk::BufferCreateInfo& bufferCreateInfo,
		const VmaAllocationCreateInfo& vmaAllocationCreateInfo);

	UniqueVmaBuffer(const UniqueVmaBuffer&) = delete;
	UniqueVmaBuffer& operator=(const UniqueVmaBuffer&) = delete;

	UniqueVmaBuffer(UniqueVmaBuffer&& rhs) noexcept;
	UniqueVmaBuffer& operator=(UniqueVmaBuffer&& rhs) noexcept;

	~UniqueVmaBuffer();

	vk::Buffer Get() { return m_buffer; }
	VmaAllocator GetAllocator() { return m_allocator; }
	VmaAllocation GetAllocation() { return m_allocation; }
private:

	vk::Buffer m_buffer;
	VmaAllocator m_allocator;
	VmaAllocation m_allocation;
};

inline UniqueVmaBuffer::UniqueVmaBuffer(VmaAllocator allocator, const vk::BufferCreateInfo& bufferCreateInfo, const VmaAllocationCreateInfo& vmaAllocationCreateInfo)
	: m_buffer(nullptr)
	, m_allocator(allocator)
	, m_allocation(nullptr)
{
	const VkBufferCreateInfo& cBufferCreateInfoRef = bufferCreateInfo;
	VkBuffer cBuffer = {};
	vmaCreateBuffer(m_allocator, &cBufferCreateInfoRef, &vmaAllocationCreateInfo, &cBuffer, &m_allocation, nullptr);
	m_buffer = cBuffer;
}

inline UniqueVmaBuffer::UniqueVmaBuffer(UniqueVmaBuffer&& rhs) noexcept
	: m_buffer(rhs.m_buffer)
	, m_allocator(rhs.m_allocator)
	, m_allocation(rhs.m_allocation)
{
	rhs.m_buffer = nullptr;
}

inline UniqueVmaBuffer& UniqueVmaBuffer::operator=(UniqueVmaBuffer&& rhs) noexcept
{
	std::swap(m_buffer, rhs.m_buffer);
	std::swap(m_allocator, rhs.m_allocator);
	std::swap(m_allocation, rhs.m_allocation);
	return *this;
}

inline UniqueVmaBuffer::~UniqueVmaBuffer()
{
	if (m_buffer)
	{
		vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
	}
}


