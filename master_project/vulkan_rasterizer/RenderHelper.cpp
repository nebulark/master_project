#include "pch.hpp"
#include "RenderHelper.hpp"
#include "NTree.hpp"

void RenderHelper::CalcStencilReferences(int maxPortals, int iterationCount,  gsl::span<uint8_t> outStencilReferences)
{
	assert(outStencilReferences.size() >= NTree::CalcTotalElements(maxPortals, iterationCount + 1));

	outStencilReferences[0] = 0;

	// we need one extra "portal" which represents "no portal"
	// real portals will start with id 1
	const uint32_t portalIdBits = GetNumBitsToStoreValue(maxPortals + 1);
	assert(portalIdBits * iterationCount < 8);
	
	for (uint32_t layer = 1; layer <= iterationCount; ++layer)
	{
		const uint32_t previousLayerStartIndex = NTree::CalcFirstLayerIndex(maxPortals, layer - 1);

		const uint32_t layerStartIndex = NTree::CalcFirstLayerIndex(maxPortals, layer);
		const uint32_t layerEndIndex = NTree::CalcFirstLayerIndex(maxPortals, layer + 1);

		assert(((layerStartIndex - previousLayerStartIndex) * maxPortals) == (layerEndIndex - layerStartIndex));
		uint32_t currentElementIndex = layerStartIndex;

		for (uint32_t parentIndex = previousLayerStartIndex; parentIndex < layerStartIndex; ++parentIndex)
		{

			// shift portal id bits to the right to make room for
			const uint8_t parentPrefix = outStencilReferences[parentIndex] << portalIdBits;

			for (uint8_t childnum = 0; childnum < maxPortals; ++childnum)
			{
				// 0 is reserved for no portal, to avoid ambiguities (e.g. parent 00 and child 00 00 would be ambiguous)

				const uint8_t childId = childnum + 1;

				// there should be any overlapping bits
				assert((childId & parentPrefix) == 0);

				outStencilReferences[currentElementIndex] = childId | parentPrefix;
				++currentElementIndex;
			}
		}

		assert(currentElementIndex == layerEndIndex);
	}
}

uint8_t RenderHelper::CalcLayerCompareMask(int maxPortals, int currentIteration)
{
	// we need one extra "portal" which represents "no portal"
	// real portals will start with id 1
	uint32_t portalBits = GetNumBitsToStoreValue(maxPortals + 1);
	return gsl::narrow<uint8_t>(ipow(2, portalBits * currentIteration));
}
