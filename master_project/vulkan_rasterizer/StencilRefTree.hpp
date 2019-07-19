#pragma once
#include <gsl/gsl>

class StencilRefTree
{
public:
	void RecalcTree(gsl::span<const int> visiblePortalCountForLayer);

	gsl::span<const uint8_t> GetStencilRefsForLayer(int layerNum) const;
	uint8_t GetLayerCompareMask(int layerNum) const { return m_compareMaskForLayer[layerNum]; }
	int GetVisiblePortalCountForLayer(int layerNum) const { return m_visiblePortalCountForLayer[layerNum]; }

	int CalcLayerStartIndex(int layerNum) const;
	int CalcLayerElementCount(int layerNum) const;

	// Calculates the number of right shifts need on the child stencil ref, so that it won't overwrite its parent stencil ref
	int CalcStencilShiftBitsForLayer(int layerNum) const;

	int GetCameraIndexBufferElementCount() const {  return m_stencilRefs.size(); }
private:
	std::vector<int> m_visiblePortalCountForLayer;

	std::vector<uint8_t> m_stencilRefs;
	std::vector<uint8_t> m_compareMaskForLayer;
};
