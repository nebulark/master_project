#pragma once
#include <gsl/gsl>

class RecursionTree
{
public:

	constexpr static int CalcLayerElementCount(gsl::index layerNum, gsl::span<const int> visiblePortalCountForLayer)
	{
		int result = 1;
		for (gsl::index i = 0; i <= layerNum; ++i)
		{
			result *= visiblePortalCountForLayer[i];
		}

		return result;
	}


	static constexpr int CalcLayerStartIndex(gsl::index layerNum, gsl::span<const int> visiblePortalCountForLayer)
	{
		int result = 1;
		for (gsl::index i = 0; i < layerNum; ++i)
		{
			result += CalcLayerElementCount(i, visiblePortalCountForLayer);
		}
		return result;
	}

	static constexpr int GetCameraIndexBufferElementCount(gsl::span<const int> visiblePortalCountForLayer) {  return CalcLayerStartIndex(visiblePortalCountForLayer.size(), visiblePortalCountForLayer); }

};
