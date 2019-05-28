#pragma once
#include <limits>
#include <type_traits>
#include <array>

template<size_t value>
constexpr auto SelectIntegerTypeDetail()
{
	if constexpr (value <= std::numeric_limits<uint8_t>::max())
	{
		return uint8_t{};
	}
	else if constexpr (value <= std::numeric_limits<uint16_t>::max())
	{
		return uint16_t{};
	}
	else if constexpr (value <= std::numeric_limits<uint32_t>::max())
	{
		return uint32_t{};
	}
	else
	{
		static_assert(value <= std::numeric_limits<uint64_t>::max(), "MaxSize too high");
		return uint64_t{};
	}
}

template<size_t MaxSize>
using SelectInteger = decltype(SelectIntegerTypeDetail<MaxSize>());
static_assert(std::is_same_v< uint8_t, SelectInteger<std::numeric_limits<uint8_t>::max()>>);
static_assert(std::is_same_v< uint16_t, SelectInteger<std::numeric_limits<uint16_t>::max()>>);
static_assert(std::is_same_v< uint32_t, SelectInteger<std::numeric_limits<uint32_t>::max()>>);
static_assert(std::is_same_v< uint64_t, SelectInteger<std::numeric_limits<uint64_t>::max()>>);
	

template<typename T, size_t SizeMax = 16>
class SmallArraySet
{
public:
	using ValueType = T;
	using IndexType = SelectInteger<SizeMax>;

	SmallArraySet() : m_numElements(0){}

	template<typename IterType>
	SmallArraySet(IterType begin, IterType end);

	ValueType* data() {return m_storage.data(); }
	const ValueType* data() const { return m_storage.data(); }
	size_t size() const { return m_numElements; }

	auto begin() {	return m_storage.begin();}
	auto end() { return m_storage.begin() + m_numElements; }

	auto cbegin() const { return m_storage.cbegin(); }
	auto cend() const { return m_storage.cbegin() + m_numElements; }

	auto begin() const { return cbegin(); }
	auto end() const { return cend(); }

	bool contains(const ValueType& value) const;
	
	bool add_unique(const ValueType& value);

	template<typename IterType>
	void add_unique(IterType begin, IterType end);


	ValueType& operator[](size_t index);
	const ValueType& operator[](size_t index) const;

private:
	std::array<ValueType, SizeMax> m_storage;
	IndexType m_numElements;	
};

template<typename T, size_t SizeMax>
template<typename IterType>
void SmallArraySet<T, SizeMax>::add_unique(IterType begin, IterType end)
{
	assert(std::distance(begin, end) + m_numElements <= SizeMax);
	for (IterType i = begin; i != end; ++i)
	{
		add_unique(*i);
	}
}

template<typename T, size_t SizeMax>
bool SmallArraySet<T, SizeMax>::add_unique(const ValueType& value)
{
	assert(m_numElements < SizeMax);
	if (!contains(value))
	{
		m_storage[m_numElements] = value;
		++m_numElements;
		return true;
	}
	return false;
}

template<typename T, size_t SizeMax>
inline bool SmallArraySet<T, SizeMax>::contains(const ValueType& value) const
{
	const auto b = cbegin();
	const auto e = cend();
	return std::find(b,e, value) != e;
}

template<typename T, size_t SizeMax>
inline typename SmallArraySet<T, SizeMax>::ValueType& SmallArraySet<T, SizeMax>::operator[](size_t index)
{
	assert(index < m_numElements);
	return m_storage[index];
}

template<typename T, size_t SizeMax>
inline const typename SmallArraySet<T, SizeMax>::ValueType& SmallArraySet<T, SizeMax>::operator[](size_t index) const
{
	assert(index < m_numElements);
	return m_storage[index];
}

template<typename T, size_t SizeMax>
template<typename IterType>
inline SmallArraySet<T,SizeMax>::SmallArraySet(IterType begin, IterType end)
	: m_numElements(0)
{
	add_unique(begin, end);
}

