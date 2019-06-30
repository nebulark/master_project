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

constexpr uint32_t GetNumBitsToStoreValue(uint32_t maxValue)
{
	uint32_t result = 0;
	while (maxValue != 0)
	{
		maxValue /= 2;
		++result;
	}
	return result;
}


class NTree
{

public:

	static constexpr uint32_t CalcFirstLayerIndex(uint32_t n, uint32_t layerNum) 
	{
		// maybe there is a sum equation for this?
		uint32_t result = 0;

		for (int i = 0; i < layerNum;++i)
		{
			result = result * 4 + 1;
		}
		return result;
	}
	static constexpr uint32_t CalcTotalElements(uint32_t n, uint32_t treeHight) { return CalcFirstLayerIndex(n, treeHight); }

	static constexpr uint32_t GetChildElementIdx(uint32_t n, uint32_t parentIdx, uint32_t childnum)
	{
		assert(childnum < n);
		return parentIdx * n + 1 + childnum;
	}

	static constexpr uint32_t GetParentIdx(uint32_t n, uint32_t childIdx)
	{
		assert(childIdx != 0);
		return (childIdx - 1) / n;
	}

	struct ParentIdxAndChildNum
	{
		uint32_t parentIdx;
		uint32_t childNum;
	};

	static constexpr ParentIdxAndChildNum GetParentIdxAndChildNum(uint32_t n, uint32_t childIdx)
	{
		assert(childIdx != 0);
		const uint32_t c = childIdx - 1;
	
		ParentIdxAndChildNum result{
			c / n,
			c % n,
		};

		return result;
	}

};

class NTreeIterator
{
public:
private:
	int currentElement;
	int currentLayer;
	int parentIndex;
	int elementCount;
};

