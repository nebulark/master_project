#pragma once
#include <type_traits>
#include <array>
#include <cassert>



// exponation by squaring
constexpr uint32_t ipow(uint32_t base, uint32_t exp)
{
	uint32_t result = 1;
	while (true)
	{
		if (exp & 1)
		{
			result *= base;
		}
		exp >>= 1;
		if (!exp)
		{
			break;
		}
		base *= base;
	}

	return result;
}

class NTree
{

public:

	static constexpr uint32_t CalcFirstLayerIndex(uint32_t n, uint32_t layerNum) { return ipow(n, layerNum) - 1; }
	static constexpr uint32_t CalcTotalElements(uint32_t n, uint32_t treeHight) { return CalcFirstLayerIndex(n, treeHight + 1); }

	static constexpr uint32_t GetChildElementIdx(uint32_t n, uint32_t parentIdx, uint32_t childnum)
	{
		assert(childnum < n);
		return parentIdx * n + 1 + childnum;
	}

	static constexpr uint32_t GetParentIdx(uint32_t n, uint32_t childIdx)
	{
		return (childIdx - 1) / n;
	}

};
