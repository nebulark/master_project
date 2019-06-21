#pragma once

#include "glm.hpp"
#include "NTree.hpp"



struct Portal
{
	int meshIndex;
	glm::mat4 a_modelmat;
	glm::mat4 b_modelmat;

	glm::mat4 a_to_b;
	glm::mat4 b_to_a;

	static Portal CreateWithModelMats(int meshIndex, const glm::mat4& a_modelmat, const glm::mat4& b_modelmat);
	static Portal CreateWithModelMatAndTranslation(int meshIndex, const glm::mat4& a_modelmat, const glm::mat4& a_to_b);

	static void CreateCameraMatrices(
		gsl::span<const Portal> portals,
		glm::mat4 CameraMatrix,
		int recursions,
		gsl::span<glm::mat4> outBuffer
	);
};

inline Portal Portal::CreateWithModelMats(int meshIndex, const glm::mat4& a_modelmat, const glm::mat4& b_modelmat)
{
	Portal result = { meshIndex, a_modelmat, b_modelmat };
	result.a_to_b = b_modelmat * glm::inverse(a_modelmat);
	result.b_to_a = glm::inverse(result.a_to_b);
	return result;
}

inline Portal Portal::CreateWithModelMatAndTranslation(int meshIndex, const glm::mat4& a_modelmat, const glm::mat4& a_to_b)
{
	Portal result;
	result.meshIndex = meshIndex;
	result.a_modelmat = a_modelmat;
	result.a_to_b = a_to_b;
	result.b_modelmat = a_to_b * a_modelmat;
	result.b_to_a = glm::inverse(a_to_b);
	return result;
}


inline void Portal::CreateCameraMatrices(
	gsl::span<const Portal> portals,
	glm::mat4 CameraMatrix,
	int recursions,
	gsl::span<glm::mat4> outBuffer)
{
	// we build an NTree, with a child for each portal connections (portals are two sided so we have to connection per element in portals)
	// each children corresponds to the product between the parent matrix and the portals translation matrix (a_to_b / b_to_a)

	// each portal struct actually defines two portals
	// this value will we uses a N for the NTree
	const uint32_t portalCount = portals.size() * 2;
	const uint32_t matrixCount = NTree::CalcTotalElements(portalCount, recursions);
	assert(outBuffer.size() >= matrixCount);

	for (uint32_t layer = 1; layer <= recursions; ++layer)
	{
		const uint32_t previousLayerStartIndex = NTree::CalcFirstLayerIndex(portalCount, layer - 1);
		const uint32_t layerStartIndex = NTree::CalcFirstLayerIndex(portalCount, layer);

		// for each parent we iterate over all portals
		for (uint32_t parentIdx = previousLayerStartIndex; parentIdx < layerStartIndex; ++parentIdx)
		{
			// we are iterating over 2 elements at once, so the actual portals are 2*i and 2*i +1!
			for (uint32_t i = 0; i < portals.size(); ++i)
			{
				const int childIdx0 = NTree::GetChildElementIdx(portalCount, parentIdx, 2 * i);
				const int childIdx1 = childIdx0 + 1;

				// validate that we don't got out of range
				assert(childIdx1 < matrixCount);

				outBuffer[childIdx0] = portals[i].a_to_b * outBuffer[parentIdx];
				outBuffer[childIdx1] = portals[i].b_to_a * outBuffer[parentIdx];
			}
		}
	}	

	// TODO: could be improved by keeping a parent and a child index and incrementing those as needed instead of recalculating all the time?
}

