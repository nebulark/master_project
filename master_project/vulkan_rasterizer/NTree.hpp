#pragma once
#include <type_traits>
#include <array>
#include <cassert>



// exponation by squaring
constexpr int ipow(uint32_t base, uint32_t exp)
{
	int result = 1;
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

	static constexpr int CalcFirstLayerIndex(uint32_t n, int layerNum) { return ipow(n, layerNum) - 1; }
	static constexpr int CalcTotalElements(uint32_t n, int treeHight) { return CalcFirstLayerIndex(n, treeHight); }

	static constexpr int GetChildElementIdx(uint32_t n, int parentIdx, int childnum)
	{
		assert(childnum < n);
		return parentIdx * n + 1 + childnum;
	}

	static constexpr int GetParentIdx(uint32_t n, int childIdx)
	{
		return (childIdx - 1) / n;
	}

};
