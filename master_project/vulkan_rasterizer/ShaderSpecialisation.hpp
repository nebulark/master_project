#pragma once

#include <cstdint>
#include <array>
#include <vulkan/vulkan.hpp>

namespace ShaderSpecialisation
{
	template<int Count>
	class MultiBytes
	{
		static_assert(Count > 0);
	public:

		MultiBytes()
		{
			data.fill(0);
			for (int i = 0; i < Count; ++i)
			{
				mapEntries[i].constantID = 0;
				mapEntries[i].size = sizeof(data[0]);
				mapEntries[i].offset = sizeof(data[0]) * i;
			}

		}
		std::array<uint8_t, Count> data;
		const std::array<vk::SpecializationMapEntry, Count>& GetMapEntries() const { return mapEntries; }
	private:
		std::array<vk::SpecializationMapEntry, Count> mapEntries;	

	};

	// The result is only valid as long as multibytes is alive!!!
	template<int Count>
	vk::SpecializationInfo ReferenceMultibytes(const MultiBytes<Count>& multibytes)
	{
		return vk::SpecializationInfo{}
			.setDataSize(sizeof(multibytes.data[0]) * multibytes.data.size())
			.setPData(multibytes.data.data())
			.setMapEntryCount(gsl::narrow<uint32_t>(multibytes.GetMapEntries().size()))
			.setPMapEntries(multibytes.GetMapEntries().data());

	}
}
