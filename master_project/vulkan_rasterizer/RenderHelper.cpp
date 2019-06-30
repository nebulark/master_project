#include "pch.hpp"
#include "RenderHelper.hpp"
#include "NTree.hpp"


namespace
{

	bool validateStencilsRefs(int maxPortals, int iterationCount, gsl::span<const uint8_t> outStencilRefereces)
	{
		for (int layer = 0; layer < iterationCount; ++layer)
		{
			const uint8_t layerMask = RenderHelper::CalcLayerCompareMask(maxPortals, layer);

			const int firstLayerElement = NTree::CalcFirstLayerIndex(maxPortals, layer);
			const int nextLayerFirstElement = NTree::CalcFirstLayerIndex(maxPortals, layer + 1);
			for (int layerelement = firstLayerElement; layerelement < nextLayerFirstElement; ++layerelement)
			{
				const int firstChild = NTree::GetChildElementIdx(maxPortals, layerelement, 0);
				const uint8_t elementVal = outStencilRefereces[layerelement];
				for (int i = 0; i < maxPortals; ++i)
				{
					const uint8_t childVal = outStencilRefereces[firstChild + i];

					bool isEqual = (elementVal & layerMask) == (childVal & layerMask);
					printf("mask: %i, %i == %i\n", layerMask, elementVal, childVal);
					assert(isEqual);
					if (!isEqual)
					{
						return false;
					}
				}

			}

		}
		return true;

	}


}



void RenderHelper::CalcStencilReferences(int maxPortals, int iterationCount,  gsl::span<uint8_t> outStencilReferences)
{
	assert(outStencilReferences.size() >= NTree::CalcTotalElements(maxPortals, iterationCount + 1));

	outStencilReferences[0] = 0;

	// we need one extra "portal" which represents "no portal"
	// real portals will start with id 1
	const uint32_t portalIdBits = GetNumBitsToStoreValue(maxPortals + 1);
	assert(portalIdBits * iterationCount <= 8);
	
	for (uint32_t layer = 1; layer <= iterationCount; ++layer)
	{
		const uint32_t previousLayerStartIndex = NTree::CalcFirstLayerIndex(maxPortals, layer - 1);

		const uint32_t layerStartIndex = NTree::CalcFirstLayerIndex(maxPortals, layer);
		const uint32_t layerEndIndex = NTree::CalcFirstLayerIndex(maxPortals, layer + 1);

		assert(((layerStartIndex - previousLayerStartIndex) * maxPortals) == (layerEndIndex - layerStartIndex));
		uint32_t currentElementIndex = layerStartIndex;

		for (uint32_t parentIndex = previousLayerStartIndex; parentIndex < layerStartIndex; ++parentIndex)
		{

			const uint8_t parentSuffix = outStencilReferences[parentIndex];

			for (uint8_t childnum = 0; childnum < maxPortals; ++childnum)
			{
				// 0 is reserved for no portal, to avoid ambiguities (e.g. parent 00 and child 00 00 would be ambiguous)

				const uint8_t childId = childnum + 1;// Why this works when setting to + 2??!!!!!!!!!!!!!!!!!!!!!
				const uint8_t childPrefix = childId << (portalIdBits * (layer-1));

				// there should be any overlapping bits
				assert((childPrefix & parentSuffix) == 0);

				outStencilReferences[currentElementIndex] = childPrefix | parentSuffix;
				++currentElementIndex;
			}
		}

		assert(currentElementIndex == layerEndIndex);
	}

	validateStencilsRefs(maxPortals, iterationCount, outStencilReferences);
}

uint8_t RenderHelper::CalcLayerCompareMask(int maxPortals, int currentIteration)
{
	// we need one extra "portal" which represents "no portal"
	// real portals will start with id 1
	const uint32_t portalBits = GetNumBitsToStoreValue(maxPortals + 1);
	return gsl::narrow<uint8_t>(ipow(2, portalBits * currentIteration) - 1);
}
