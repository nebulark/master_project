#include "pch.hpp"
#include "RecursionTree.hpp"


int RecursionTree::CalcLayerStartIndex(int layerNum, gsl::span<const int> visiblePortalCountForLayer) 
{
	int result = 1;
	for (int i = 0; i < layerNum; ++i)
	{
		result += CalcLayerElementCount(i, visiblePortalCountForLayer);
	}
	return result;
}

int RecursionTree::CalcLayerElementCount(int layerNum, gsl::span<const int> visiblePortalCountForLayer) 
{
	return std::reduce(
		visiblePortalCountForLayer.cbegin(),
		visiblePortalCountForLayer.cbegin() + layerNum + 1,
		1, std::multiplies<>())
		;
}

