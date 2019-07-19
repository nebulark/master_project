#pragma once
#include <gsl/gsl>

class RecursionTree
{
public:

	static int CalcLayerStartIndex(int layerNum, gsl::span<const int> visiblePortalCountForLayer);
	static int CalcLayerElementCount(int layerNum, gsl::span<const int> visiblePortalCountForLayer);

	static int GetCameraIndexBufferElementCount(gsl::span<const int> visiblePortalCountForLayer) {  return CalcLayerStartIndex(visiblePortalCountForLayer.size(), visiblePortalCountForLayer); }

};
