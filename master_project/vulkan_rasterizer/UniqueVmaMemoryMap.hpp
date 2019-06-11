#pragma once

#include "VulkanMemoryAllocator/vk_mem_alloc.h"

class UniqueVmaMemoryMap
{
public:
	UniqueVmaMemoryMap(VmaAllocator allocator, VmaAllocation allocation);

	UniqueVmaMemoryMap(const UniqueVmaMemoryMap&) = delete;
	UniqueVmaMemoryMap& operator=(const UniqueVmaMemoryMap&) = delete;

	UniqueVmaMemoryMap(UniqueVmaMemoryMap&& rhs) noexcept;
	UniqueVmaMemoryMap& operator=(UniqueVmaMemoryMap&& rhs) noexcept;

	~UniqueVmaMemoryMap();

	std::byte* GetMappedMemoryPtr() { return m_mappedMemory; }
private:

	std::byte* m_mappedMemory;
	VmaAllocator m_allocator;
	VmaAllocation m_allocation;
};
