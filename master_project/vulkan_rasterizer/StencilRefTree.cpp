#include "pch.hpp"
#include "StencilRefTree.hpp"
#include "NTree.hpp"

void StencilRefTree::RecalcTree(gsl::span<const int> visiblePortalCountForLayer)
{
	m_visiblePortalCountForLayer.resize(visiblePortalCountForLayer.size());
	std::copy(visiblePortalCountForLayer.cbegin(), visiblePortalCountForLayer.cend(), m_visiblePortalCountForLayer.begin());

	m_compareMaskForLayer.resize(visiblePortalCountForLayer.size() + 1);

	// start index of next layer defines the number of elements we need 
	const int totalStencilRefCount = CalcLayerStartIndex(visiblePortalCountForLayer.size());

	m_stencilRefs.resize(totalStencilRefCount);
	
	int currentStencilRefIdx = 0;


	int bitsToShift = 0;

	// first layer
	{		
		constexpr int firstLayer = 0;
		const int bitsForFirstLayer = GetNumBitsToStoreValue(visiblePortalCountForLayer[firstLayer] + 1);

		// shifting adds zeroes to the end, but we need ones at the end, thats why we need the complement
		const uint8_t layerCompareMask = ~((0b1111'1111u) << bitsForFirstLayer);
		m_compareMaskForLayer[firstLayer] = layerCompareMask;

		// first itration, no bitshifting needed
		assert(bitsToShift == 0);
		for (int childNum = 0, portalCount = visiblePortalCountForLayer[firstLayer]; childNum < portalCount; ++childNum)
		{
			// 0 is reserved so start with 1
			m_stencilRefs[currentStencilRefIdx++] = gsl::narrow<uint8_t>(childNum + 1);
		}

		bitsToShift += bitsForFirstLayer;
	}

	int previousLayerStartIndex = 0;

	// subsequentlayers
	for (int layer = 1, layercount = visiblePortalCountForLayer.size(); layer < layercount; ++layer)
	{
		const int previousLayerElements = CalcLayerElementCount(layer - 1);
		const int layerStartIndex = previousLayerStartIndex + previousLayerElements;

		const int bitsForLayer = GetNumBitsToStoreValue(visiblePortalCountForLayer[layer] + 1);

		// shifting adds zeroes to the end, but we need ones at the end, thats why we need the complement
		const uint8_t layerCompareMask = ~((0b1111'1111u) << (bitsToShift+bitsForLayer) );
		m_compareMaskForLayer[layer] = layerCompareMask;
	
		// for each parent we iterate over all portals
		for (int parentIdx = previousLayerStartIndex; parentIdx < layerStartIndex; ++parentIdx)
		{

			const uint8_t parentStencilRef = m_stencilRefs[parentIdx];

			// there should not be a bit set outside the compare mask
			assert((parentStencilRef & (~layerCompareMask)) == 0);

			// there must be a bit set somewhere, as we don't allow a value of 0
			assert(parentStencilRef != 0);

			for (int childNum = 0, portalCount = visiblePortalCountForLayer[layer]; childNum < portalCount; ++childNum)
			{
				const uint8_t childStencilRef = gsl::narrow<uint8_t>(childNum + 1);
				const uint8_t shiftedChildStencilRef = childStencilRef << bitsToShift;

				// parent and shifted child stencil ref never overlap
				assert((parentStencilRef & shiftedChildStencilRef) == 0);

				const uint8_t stencilRef = parentStencilRef | shiftedChildStencilRef;
								
				m_stencilRefs[currentStencilRefIdx++] = stencilRef;
			}
		}

		bitsToShift += bitsForLayer;
		previousLayerStartIndex = layerStartIndex;
	}
}

gsl::span<const uint8_t> StencilRefTree::GetStencilRefsForLayer(int layerNum) const
{
	const int layerStartIndex = CalcLayerStartIndex(layerNum);
	const uint8_t* layerStartPtr = &m_stencilRefs[layerStartIndex];
	const int layerCount = CalcLayerElementCount(layerNum);
	
	return gsl::make_span(layerStartPtr, layerCount);
}

int StencilRefTree::CalcLayerStartIndex(int layerNum) const
{
	int result = 0;
	for (int i = 0; i < layerNum; ++i)
	{
		result += CalcLayerElementCount(i);
	}
	return result;
}

int StencilRefTree::CalcLayerElementCount(int layerNum) const
{
	return std::reduce(
		m_visiblePortalCountForLayer.cbegin(),
		m_visiblePortalCountForLayer.cbegin() + layerNum + 1,
		1, std::multiplies<>())
		;
}

int StencilRefTree::CalcStencilShiftBitsForLayer(int layerNum) const
{
	int numBitsToShift = 0;
	for (int i = 0; i < layerNum; ++i)
	{
		numBitsToShift += GetNumBitsToStoreValue(m_visiblePortalCountForLayer[i] + 1);
	}

	return numBitsToShift;
}
