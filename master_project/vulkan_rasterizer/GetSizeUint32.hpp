#pragma once
#include <cstdint>

	template<typename T>
	uint32_t GetSizeUint32(const T& Container) noexcept
	{
		return gsl::narrow_cast<uint32_t>(std::size(Container));
	}

	template<typename T, size_t Size>
	uint32_t GetSizeUint32(const T(&array)[Size]) noexcept
	{
		return gsl::narrow_cast<uint32_t>(std::size(array));
	}

