#pragma once
#include <type_traits>

template<typename enum_type, enum_type enum_first, enum_type enum_last>
class EnumIndex
{
private:
	// used for arithmetic operations
	using StorageType = gsl::index;
	static constexpr StorageType internal_first = static_cast<StorageType>(enum_first);
	static constexpr StorageType internal_last = static_cast<StorageType>(enum_last);

	StorageType internal_value;

public:
	using Self = EnumIndex<enum_type, enum_first, enum_last>;

	explicit constexpr EnumIndex(enum_type value) : internal_value(static_cast<StorageType>(value)){}
	static constexpr EnumIndex First() { return EnumIndex(enum_first); }
	static constexpr EnumIndex Last() { return EnumIndex(enum_last); }

	static constexpr gsl::index GetRange() { return static_cast<gsl::index>(internal_last) - static_cast<gsl::index>(internal_first) + 1; }

	gsl::index ToIndex() const { return static_cast<gsl::index>(internal_value); }
	operator gsl::index() const { return ToIndex(); }
	operator enum_type() const { return static_cast<enum_type>(internal_value); }

	EnumIndex& operator++();
	EnumIndex operator++(int);
	
	EnumIndex& operator--();
	EnumIndex operator--(int);

	bool operator<(enum_type rhs) const;
	bool operator>=(enum_type rhs) const;
	bool operator<=(enum_type rhs) const;
	bool operator>(enum_type rhs) const;


private:

};



template<typename enum_type, enum_type enum_first, enum_type enum_last>
inline EnumIndex<enum_type, enum_first, enum_last>& EnumIndex<enum_type, enum_first, enum_last>::operator++()
{
	assert(internal_value != (internal_last + 1));
	++internal_value;
	return *this;
}

template<typename enum_type, enum_type enum_first, enum_type enum_last>
inline EnumIndex<enum_type, enum_first, enum_last> EnumIndex<enum_type, enum_first, enum_last>::operator++(int)
{
	const auto old = *this;
	++(*this);
	return old;
}

template<typename enum_type, enum_type enum_first, enum_type enum_last>
inline EnumIndex<enum_type, enum_first, enum_last>& EnumIndex<enum_type, enum_first, enum_last>::operator--()
{
	assert(internal_value != (internal_first - 1));
	--internal_value;
	return *this;
}

template<typename enum_type, enum_type enum_first, enum_type enum_last>
inline EnumIndex<enum_type, enum_first, enum_last> EnumIndex<enum_type, enum_first, enum_last>::operator--(int)
{
	const auto old = *this;
	--(*this);
	return old;
}

template<typename enum_type, enum_type enum_first, enum_type enum_last>
inline bool EnumIndex<enum_type, enum_first, enum_last>::operator<(enum_type rhs) const
{
	return internal_value < static_cast<StorageType>(rhs);
}

template<typename enum_type, enum_type enum_first, enum_type enum_last>
inline bool EnumIndex<enum_type, enum_first, enum_last>::operator>=(enum_type rhs) const
{
	return !((*this) < rhs);
}

template<typename enum_type, enum_type enum_first, enum_type enum_last>
inline bool EnumIndex<enum_type, enum_first, enum_last>::operator>(enum_type rhs) const
{
	return internal_value > static_cast<StorageType>(rhs);
}

template<typename enum_type, enum_type enum_first, enum_type enum_last>
inline bool EnumIndex<enum_type, enum_first, enum_last>::operator<=(enum_type rhs) const
{
	return !((*this) > rhs);
}
