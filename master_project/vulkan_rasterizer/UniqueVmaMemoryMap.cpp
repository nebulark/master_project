#include "pch.hpp"
#include "UniqueVmaMemoryMap.hpp"

UniqueVmaMemoryMap::UniqueVmaMemoryMap(VmaAllocator allocator, VmaAllocation allocation)
	:m_mappedMemory(nullptr)
	,m_allocator(allocator)
	,m_allocation(allocation)
{
	void* mappedMemory;
	VkResult result = vmaMapMemory(m_allocator, m_allocation, &mappedMemory);
	m_mappedMemory = reinterpret_cast<std::byte*>(mappedMemory);
}


UniqueVmaMemoryMap::UniqueVmaMemoryMap(UniqueVmaMemoryMap&& rhs) noexcept
	:m_mappedMemory(rhs.m_mappedMemory)
	,m_allocator(rhs.m_allocator)
	,m_allocation(rhs.m_allocation)
{
	rhs.m_mappedMemory = nullptr;
}

UniqueVmaMemoryMap& UniqueVmaMemoryMap::operator=(UniqueVmaMemoryMap&& rhs) noexcept
{
	std::swap(m_mappedMemory, rhs.m_mappedMemory);
	std::swap(m_allocator, rhs.m_allocator);
	std::swap(m_allocation, rhs.m_allocation);
	return *this;
}

UniqueVmaMemoryMap::~UniqueVmaMemoryMap()
{
	if (m_mappedMemory != nullptr)
	{
		vmaUnmapMemory(m_allocator, m_allocation);
	}
}
