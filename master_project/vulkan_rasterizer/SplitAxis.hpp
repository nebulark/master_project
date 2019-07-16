#pragma once


class SplitAxis
{
public:
	using InternalValue_t = int_fast8_t;

	explicit constexpr SplitAxis(InternalValue_t internalValue) : internalValue(internalValue){ assert(0 <= internalValue && internalValue <= 3);}
	static constexpr SplitAxis FromDim(int dim) { assert(0 <= dim && dim <= 2); return SplitAxis(dim); }

	static constexpr SplitAxis dim_x() { return FromDim(0); };
	static constexpr SplitAxis dim_y() { return FromDim(1); };
	static constexpr SplitAxis dim_z() { return FromDim(2); };

	static constexpr SplitAxis leafNode() { return SplitAxis(3); }


	static constexpr int StorageBitCount = 2;

	constexpr bool IsLeafNode() const { return *this == leafNode(); }
	constexpr InternalValue_t ToDim() const { assert(0 <= internalValue && internalValue <= 2); return internalValue; };
	constexpr InternalValue_t GetInternalValue() const { return internalValue; }
	constexpr SplitAxis NextAxis() const { return FromDim((internalValue + 1) % 3); }
private:

	InternalValue_t internalValue;
	friend constexpr bool operator==(SplitAxis lhs, SplitAxis rhs);
};

inline constexpr bool operator==(SplitAxis lhs, SplitAxis rhs)
{
	return lhs.internalValue == rhs.internalValue;
}

inline constexpr bool operator!=(SplitAxis lhs, SplitAxis rhs)
{
	return !(lhs == rhs);
}


