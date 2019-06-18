#pragma once
#include <cstdint>
#include <array>
#include <type_traits>

template< class, class = void >
struct HasSizeMemberFunction : std::false_type
{ };

// specialized as has_member< T , void > or discarded (sfinae)
template<class T>
struct HasSizeMemberFunction< T, std::void_t<decltype(&T::size) >> : std::true_type
{ };

template<typename T>
constexpr bool HasStdSize_v = HasSizeMemberFunction<T>::value;

template<typename T>
std::enable_if_t<HasStdSize_v<T>, uint32_t>
GetSizeUint32(const T& Container) noexcept
{
	return gsl::narrow_cast<uint32_t>(std::size(Container));
}

template<typename T>
constexpr std::enable_if_t<!HasStdSize_v<T>, uint32_t>
GetSizeUint32(const T& Container) noexcept
{
	return 1;
}

template<typename innerType, size_t Size>
constexpr uint32_t GetSizeUint32(const std::array<innerType, Size>& Container) noexcept
{
	return gsl::narrow<uint32_t>(Size);
}


template<typename T, size_t Size>
constexpr uint32_t GetSizeUint32(const T(&array)[Size]) noexcept
{
	return gsl::narrow_cast<uint32_t>(Size);
}

