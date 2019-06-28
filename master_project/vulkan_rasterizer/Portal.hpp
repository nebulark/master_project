#pragma once

#include "glm.hpp"
#include "NTree.hpp"
#include "GetSizeUint32.hpp"
#include "Transform.hpp"



struct Portal
{
	int meshIndex;
	Transform a_transform;
	Transform b_transform;

	Transform a_to_b;
	Transform b_to_a;

	static Portal CreateWithPortalTransforms(int meshIndex, const Transform& a_transform, const Transform& b_transform);
	static Portal CreateWithTransformAndAtoB(int meshIndex, const Transform& a_transform, const Transform& a_to_b);

	static void CreateCameraTransforms(
		gsl::span<const Portal> portals,
		Transform cameraTransform,
		int recursions,
		gsl::span<Transform> outTransforms);
};

inline Portal Portal::CreateWithPortalTransforms(int meshIndex, const Transform& a_transform, const Transform& b_transform)
{
	Portal result = { meshIndex, a_transform, b_transform };
	result.a_to_b = Transform::CalcAtToB(a_transform, b_transform);
	result.b_to_a = result.a_to_b.CalcInversion();
	return result;
}

inline Portal Portal::CreateWithTransformAndAtoB(int meshIndex, const Transform& a_modelmat, const Transform& a_to_b)
{
	Portal result;
	result.meshIndex = meshIndex;
	result.a_transform = a_modelmat;
	result.a_to_b = a_to_b;
	result.b_transform = a_to_b * a_modelmat;
	result.b_to_a = a_to_b.CalcInversion();
	return result;
}


inline void Portal::CreateCameraTransforms(
	gsl::span<const Portal> portals,
	Transform cameraTransform,
	int recursions,
	gsl::span<Transform> outTransforms)
{
	// we build an NTree, with a child for each portal connections (portals are two sided so we have to connection per element in portals)
	// each children corresponds to the product between the parent matrix and the portals translation matrix (a_to_b / b_to_a)

	// each portal struct actually defines two portals
	// this value will we uses a N for the NTree
	const uint32_t portalCount = GetSizeUint32(portals.size()) * 2;
	const uint32_t matrixCount = NTree::CalcTotalElements(portalCount, recursions);
	assert(outTransforms.size() >= matrixCount);

	outTransforms[0] = cameraTransform;
	for (uint32_t layer = 1; layer <= static_cast<uint32_t>(recursions); ++layer)
	{
		const uint32_t previousLayerStartIndex = NTree::CalcFirstLayerIndex(portalCount, layer - 1);
		const uint32_t layerStartIndex = NTree::CalcFirstLayerIndex(portalCount, layer);

		// for each parent we iterate over all portals
		for (uint32_t parentIdx = previousLayerStartIndex; parentIdx < layerStartIndex; ++parentIdx)
		{
			// we are iterating over 2 elements at once, so the actual portals are 2*i and 2*i +1!
			for (uint32_t i = 0; i < portals.size(); ++i)
			{
				const uint32_t childIdx0 = NTree::GetChildElementIdx(portalCount, parentIdx, 2 * i);
				const uint32_t childIdx1 = childIdx0 + 1;

				// validate that we don't got out of range
				assert(childIdx1 < matrixCount);

				outTransforms[childIdx0] = portals[i].a_to_b * outTransforms[parentIdx];
				outTransforms[childIdx1] = portals[i].b_to_a * outTransforms[parentIdx];
			}
		}
	}

	// TODO: could be improved by keeping a parent and a child index and incrementing those as needed instead of recalculating all the time?
}

