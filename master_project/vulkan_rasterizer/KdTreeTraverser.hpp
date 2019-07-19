#pragma once

#include "KdTree.hpp"
#include <optional>
#include "glm.hpp"
#include "Ray.hpp"

namespace KdTreeTraverser
{

	using RayIntersectionFunction = std::optional<float>(*)(const Ray& ray, const Triangle& triangle, void* userPtr);


	struct RayTraceResult
	{
		float t;
		int index;
	};


	struct RayTraceData
	{
		const KdTree* tree;
		Ray ray;
		RayIntersectionFunction intersectionFunction;
		void* userData;
		gsl::span<const Triangle> dataElements;
	};

	inline std::optional<RayTraceResult> RayTrace(
		const RayTraceData& rayTraceData,
		const KdNode& node,
		float tmax, float tmin = 0.f)
	{
		const SplitAxis splitAxis = node.GetSplitAxis();

		assert(rayTraceData.tree);
		const KdTree& tree = *rayTraceData.tree;

		// handle child node
		if (splitAxis.IsLeafNode())
		{
			const DataIndicesIndexView indexView = node.GetLeafIndexView();

			std::optional<RayTraceResult> result;
			for (int index : tree.GetDataIndices(indexView))
			{
				const Triangle& triangle = rayTraceData.dataElements[index];
				std::optional<float> rayTrace = rayTraceData.intersectionFunction(
					rayTraceData.ray,
					triangle,
					rayTraceData.userData);
				if (!rayTrace.has_value())
				{
					continue;
				}

				const float rayTraceValue = rayTrace.value();
				constexpr float tolerance = 1e-7f;
				if (rayTraceValue < tmin - tolerance || rayTraceValue > tmax + tolerance)
				{
					continue;
				}

				// result is better as current ray trace, ignore ray trace
				if (result.has_value() && (result->t <= rayTraceValue))
				{
					continue;
				}

				result = RayTraceResult{ rayTraceValue, index };
			}
			return result;
		}

		// Handle non-leaf node
		const float currentPosInDim = rayTraceData.ray.origin[splitAxis.ToDim()] + rayTraceData.ray.direction[splitAxis.ToDim()] * tmin;
		const float currentSplitValue = node.GetSplitVal();

		const NodePairIndex childIndexPair = node.GetChildNodeIndexPair();
		const NodeIndex childIndices[2] = { childIndexPair.GetFirstIndex(), childIndexPair.GetSecondIndex() };

		const int childIndexToTraverseFirst = currentPosInDim < currentSplitValue ? 0 : 1;
		const int childIndexToTraverseSecond = childIndexToTraverseFirst ^ 1;

		const KdNode& firstChildNodeToTraverse = tree.GetNode(childIndices[childIndexToTraverseFirst]);

		// in such a case we can never pass the plane, so we don't need to check for triangle behind the plane
		if (rayTraceData.ray.direction[splitAxis.ToDim()] == 0.f)
		{
			return  RayTrace(rayTraceData, firstChildNodeToTraverse, tmin, tmax);
		}

		const float tIntersection = (currentSplitValue - currentPosInDim) / rayTraceData.ray.direction[splitAxis.ToDim()];

		// when the intersection happens before the ray origin or after the ray end, we can never pass the plane and only need to check the first part
		if (tIntersection < tmin || tIntersection > tmax)
		{
			return RayTrace(rayTraceData, firstChildNodeToTraverse, tmin, tmax);
		}


		std::optional<RayTraceResult> firstTraceResult = RayTrace(rayTraceData, firstChildNodeToTraverse, tIntersection, tmin);

		// if successful we can skip early
		if (firstTraceResult)
		{
			return firstTraceResult;
		}

		const KdNode& secondChildNodeToTraverse = tree.GetNode(childIndices[childIndexToTraverseSecond]);
		return  RayTrace(rayTraceData, secondChildNodeToTraverse, tmax, tIntersection);
	}

}
