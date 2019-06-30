#pragma once

#include <cstdint>
#include <gsl/gsl>



namespace RenderHelper
{

	void CalcStencilReferences(int maxPortals, int iterationCount, gsl::span<uint8_t> outStencilReferences);
	uint8_t CalcLayerCompareMask(int maxPortals, int currentIteration);
};

