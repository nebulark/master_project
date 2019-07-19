#pragma once
#include <gsl/gsl>

class RecursionTree
{
public:

	constexpr static int CalcLayerElementCount(int layerNum, gsl::span<const int> visiblePortalCountForLayer)
	{
		int result = 1;
		for (int i = 0; i <= layerNum; ++i)
		{
			result *= visiblePortalCountForLayer[i];
		}

		return result;
	}


	static constexpr int CalcLayerStartIndex(int layerNum, gsl::span<const int> visiblePortalCountForLayer)
	{
		int result = 1;
		for (int i = 0; i < layerNum; ++i)
		{
			result += CalcLayerElementCount(i, visiblePortalCountForLayer);
		}
		return result;
	}

	static constexpr int GetCameraIndexBufferElementCount(gsl::span<const int> visiblePortalCountForLayer) {  return CalcLayerStartIndex(visiblePortalCountForLayer.size(), visiblePortalCountForLayer); }

};
