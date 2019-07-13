#pragma once
#include "glm.hpp"
#include "KdTreeUtils.hpp"
#include <vector>
#include <gsl/gsl>
#include "AABB.hpp"
#include "Triangle.hpp"



class KdTree
{
public:
	static constexpr int MaxIndicesPerNode = 8;

	void Init(gsl::span<const Triangle> data, const AABB& triangleBoundingBox = InvalidAABB);

	const KdNode& GetRootNode() const { return m_firstNode; }
	const KdNode& GetNode(NodeIndex nodeIndex) const { return m_nodeMemory.Get(nodeIndex); }

	gsl::span<const int> GetDataIndices(DataIndicesIndexView indicesView) const;
private:

	void InitRescursion(KdNode& inOutnode, const AABB& boundingBox, gsl::span<const Triangle> dataElements, int splitDim, std::vector<int>&& indices);

	KdNode m_firstNode;
	KdNodeMemory m_nodeMemory;

	std::vector<int> m_dataIndices;
};

namespace KdTreeDetail
{


	inline float FindSplitValue(SplitAxis axis, std::vector<int>& elementIndices, gsl::span<const Triangle> dataElements)
	{
	const int dim = static_cast<int>(axis);

		const auto GetCentroid = [dim](const Triangle& tri)
		{
			const float sum = tri.vertices[0][dim] + tri.vertices[1][dim] + tri.vertices[2][dim];
			return sum / 3.f;
		};

		const auto begin = std::begin(elementIndices);
		const auto median = begin + (std::size(elementIndices) / 2);
		const auto end = std::end(elementIndices);

		std::nth_element(begin, median, end, [dataElements, &GetCentroid](int a_index, int b_index)
			{
				const Triangle& tri_a = dataElements[a_index];
				const Triangle& tri_b = dataElements[b_index];
				const float c_a = GetCentroid(tri_a);
				const float c_b = GetCentroid(tri_b);

				return c_a < c_b;
			});

		const Triangle& medianTri =	dataElements[*median];

		const float splitValue = GetCentroid(medianTri);
		return splitValue;
	}

	struct SplitResult
	{
		std::vector<int> firstDataIndices;
		std::vector<int> secondDataIndices;
	};

	inline SplitResult Split(SplitAxis axis, float splitValue, std::vector<int>&& elementIndices, gsl::span<const Triangle> dataElements)
	{
		const int dim = static_cast<int>(axis);

		SplitResult result;

		result.firstDataIndices.reserve(elementIndices.size());
		result.secondDataIndices.reserve(elementIndices.size());

		// TODO: remove those vectors after debugging
		std::vector<Triangle> firstTris;
		std::vector<Triangle> secondsTris;
		for (int elementIndex : elementIndices)
		{
			const Triangle& tri = dataElements[elementIndex];


			if (   tri.vertices[0][dim] < splitValue
				|| tri.vertices[1][dim] < splitValue
				|| tri.vertices[2][dim] < splitValue)
			{
				result.firstDataIndices.push_back(elementIndex);
				firstTris.push_back(tri);
			}

			if (   tri.vertices[0][dim] > splitValue
				|| tri.vertices[1][dim] > splitValue
				|| tri.vertices[2][dim] > splitValue)
			{
				result.secondDataIndices.push_back(elementIndex);
				secondsTris.push_back(tri);
			}
		}

		return result;
	}
}

inline void KdTree::Init(gsl::span<const Triangle> triangles, const AABB& triangleBoundingBox /*= InvalidAABB*/)
{
	const uint32_t elementCount = triangles.size();

	// special case 0 elements
	if (elementCount == 0)
	{
		m_firstNode = KdNode::CreateLeaf(DataIndicesIndexView{ 0, 0 });
		return;
	}

	// special case, to few elements to create "real" kdtree
	if (elementCount <= MaxIndicesPerNode)
	{
		m_dataIndices.resize(elementCount);

		for (int i = 0; i < m_dataIndices.size(); ++i)
		{
			m_dataIndices[i] = i;
		}

		m_firstNode = KdNode::CreateLeaf(DataIndicesIndexView{ 0, elementCount });
		assert(m_dataIndices.size() == triangles.size());

		return;
	}


	AABB totalBoundingBox(triangleBoundingBox);
	if (totalBoundingBox == InvalidAABB)
	{
		totalBoundingBox = Triangle::CreateAABB(triangles);
	}


	std::vector<int> initialDataIndices;
	initialDataIndices.resize(elementCount);
	for (int i = 0; i < elementCount; ++i)
	{
		initialDataIndices[i] = i;
	}


	InitRescursion(m_firstNode, totalBoundingBox, triangles, 0, std::move(initialDataIndices));
}

inline void KdTree::InitRescursion(KdNode& inOutnode, const AABB& boundingBox, gsl::span<const Triangle> dataElements, int splitDim, std::vector<int>&& indices)
{
	const uint32_t indicesSize = indices.size();

	if (indicesSize <= MaxIndicesPerNode)
	{
		const uint32_t firstIndicesIndex = m_dataIndices.size();
		m_dataIndices.insert(m_dataIndices.end(), std::begin(indices), std::end(indices));

		assert(m_dataIndices.size() == firstIndicesIndex + indicesSize);

		inOutnode = KdNode::CreateLeaf(DataIndicesIndexView{ firstIndicesIndex, indicesSize });
		return;
	}



	// split at widest dimension
	SplitAxis widestDim = boundingBox.FindWidestDim();

	float splitval = (boundingBox.maxBounds[splitDim] + boundingBox.minBounds[splitDim]) / 2.f;
	splitval = KdTreeDetail::FindSplitValue(widestDim, indices, dataElements);

	KdTreeDetail::SplitResult splitResult = KdTreeDetail::Split(widestDim, splitval, std::move(indices), dataElements);

	const NodePairIndex childIndexPair = m_nodeMemory.AllocNodePair();

	inOutnode = KdNode::CreateNode(childIndexPair, widestDim, splitval);

	AABB firstBoundingBox(boundingBox);
	firstBoundingBox.maxBounds[static_cast<int>(widestDim)] = splitval;

	AABB secondBoundingBox(boundingBox);
	secondBoundingBox.minBounds[static_cast<int>(widestDim)] = splitval;

	int nextSplitDim = (splitDim + 1) % 3;

	InitRescursion(m_nodeMemory.Access(childIndexPair.GetFirstIndex()), firstBoundingBox, dataElements, nextSplitDim, std::move(splitResult.firstDataIndices));
	InitRescursion(m_nodeMemory.Access(childIndexPair.GetSecondIndex()), secondBoundingBox, dataElements, nextSplitDim, std::move(splitResult.secondDataIndices));
}

inline gsl::span<const int> KdTree::GetDataIndices(DataIndicesIndexView indicesView) const
{
	assert(m_dataIndices.size() >= indicesView.firstIndex.internalIndex + indicesView.size);
	return gsl::span<const int>(&(m_dataIndices[indicesView.firstIndex.internalIndex]), indicesView.size);
}

