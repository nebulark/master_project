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
	using DataIndex_t = uint32_t;

	void Init(gsl::span<const Triangle> data, const AABB& triangleBoundingBox = InvalidAABB);

	const KdNode& GetRootNode() const { return m_firstNode; }
	const KdNode& GetNode(NodeIndex nodeIndex) const { return m_nodeMemory.Get(nodeIndex); }

	gsl::span<const DataIndex_t> GetDataIndices(DataIndicesIndexView indicesView) const;
private:

	KdNode CreateNodeRecursive(const AABB& boundingBox, gsl::span<const Triangle> dataElements, SplitAxis currentSplitAxis, SplitAxis lastSplitAxis, std::vector<DataIndex_t>&& indices);

	KdNode m_firstNode;
	KdNodeMemory m_nodeMemory;

	std::vector<DataIndex_t> m_dataIndices;
};

namespace DetailKdTree
{


	inline float FindSplitValue(SplitAxis axis, std::vector<KdTree::DataIndex_t>& elementIndices, gsl::span<const Triangle> dataElements)
	{
		const int dim = axis.ToDim();

		const auto GetCentroid = [dim](const Triangle& tri)
		{
			const float sum = tri.vertices[0][dim] + tri.vertices[1][dim] + tri.vertices[2][dim];
			return sum / 3.f;
		};

		const auto begin = std::begin(elementIndices);
		const auto median = begin + (std::size(elementIndices) / 2);
		const auto end = std::end(elementIndices);

		std::nth_element(begin, median, end, [dataElements, &GetCentroid](KdTree::DataIndex_t a_index, KdTree::DataIndex_t b_index)
			{
				const Triangle& tri_a = dataElements[a_index];
				const Triangle& tri_b = dataElements[b_index];
				const float c_a = GetCentroid(tri_a);
				const float c_b = GetCentroid(tri_b);

				return c_a < c_b;
			});

		const Triangle& medianTri = dataElements[*median];

		const float splitValue = GetCentroid(medianTri);
		return splitValue;
	}

	inline std::optional<float> FindSplitValue_SAH(SplitAxis axis, float minSplit, float maxSplit, float minSAH, std::vector<KdTree::DataIndex_t>& elementIndices, gsl::span<const Triangle> dataElements)
	{
		const int dim = axis.ToDim();
		constexpr int sampleCount = 8;
		constexpr float inverseSampleCount = 1.f / (sampleCount);
		constexpr float intersect_cost = 1.f;
		const float splitWidth = (maxSplit - minSplit);
		const float splitWidthPerSample = splitWidth * inverseSampleCount;

		// if the split width is too small we get floating point problems
		constexpr float minSplitWidt = 1e-05f;
		if (splitWidth < minSplitWidt)
		{
			return std::nullopt;
		}

		float bestSAH = std::numeric_limits<float>::max();
		int bestSampleId = 0;

		// start with 1, as it is impossible that its best to split a min point
		for (int sampleId = 0; sampleId < sampleCount; ++sampleId)
		{
			const float probabilityFirst = sampleId * inverseSampleCount;
			const float probabilitySecond = 1.f - probabilityFirst;
			const float splitPos = minSplit + splitWidthPerSample * sampleId;

			int firstTriangleCount = 0;
			int secondTriangleCount = 0;
			for (int elementIndex : elementIndices)
			{
				const Triangle& tri = dataElements[elementIndex];

				if (tri.vertices[0][dim] < splitPos
					|| tri.vertices[1][dim] < splitPos
					|| tri.vertices[2][dim] < splitPos)
				{
					++firstTriangleCount;
				}

				if (tri.vertices[0][dim] > splitPos
					|| tri.vertices[1][dim] > splitPos
					|| tri.vertices[2][dim] > splitPos)
				{
					++secondTriangleCount;
				}
			}

		
			const float currentSAH = (probabilityFirst * firstTriangleCount) + (probabilitySecond * secondTriangleCount);

			if (currentSAH < bestSAH)
			{
				bestSAH = currentSAH;
				bestSampleId = sampleId;
			}
		}

		// if 0 is best this means no split would be best. This happens if a split would create to many duplicates
		// if we can beat the min SAH it is good enough and we stop splitting
		if (bestSampleId == 0 || bestSAH < minSAH)
		{
			return std::nullopt;
		}
		else
		{
			return minSplit + splitWidthPerSample * bestSampleId;
		}
	}


	struct SplitResult
	{
		std::vector<uint32_t> firstDataIndices;
		std::vector<uint32_t> secondDataIndices;
	};

	inline SplitResult Split(SplitAxis axis, float splitValue, std::vector<KdTree::DataIndex_t>&& elementIndices, gsl::span<const Triangle> dataElements)
	{
		const int dim = axis.ToDim();

		SplitResult result;

		result.firstDataIndices.reserve(elementIndices.size());
		result.secondDataIndices.reserve(elementIndices.size());

		// TODO: remove those vectors after debugging
		std::vector<Triangle> firstTris;
		std::vector<Triangle> secondsTris;
		for (int elementIndex : elementIndices)
		{
			const Triangle& tri = dataElements[elementIndex];

			if (tri.vertices[0][dim] < splitValue
				|| tri.vertices[1][dim] < splitValue
				|| tri.vertices[2][dim] < splitValue)
			{
				result.firstDataIndices.push_back(elementIndex);
				firstTris.push_back(tri);
			}

			if (tri.vertices[0][dim] > splitValue
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
	const DataIndex_t elementCount = gsl::narrow<DataIndex_t>(triangles.size());

	AABB totalBoundingBox(triangleBoundingBox);
	if (totalBoundingBox == InvalidAABB)
	{
		totalBoundingBox = Triangle::CreateAABB(triangles);
	}

	std::vector<DataIndex_t> initialDataIndices;
	initialDataIndices.resize(elementCount);
	for (DataIndex_t i = 0; i < elementCount; ++i)
	{
		initialDataIndices[i] = i;
	}


	constexpr SplitAxis lastSplitAxis = SplitAxis::dim_z();
	constexpr SplitAxis splitAxis = lastSplitAxis.NextAxis();

	m_firstNode = CreateNodeRecursive(totalBoundingBox, triangles, splitAxis, lastSplitAxis, std::move(initialDataIndices));
}

inline KdNode KdTree::CreateNodeRecursive(const AABB& boundingBox, gsl::span<const Triangle> dataElements, SplitAxis currentSplitAxis, SplitAxis lastSplitAxis, std::vector<DataIndex_t>&& indices)
{

	// we are currently not using kd tree
	if (true || indices.size() == 0)
	{
		return KdNode::CreateLeaf(DataIndicesIndexView{ 0, 0 });
	}


	const DataIndex_t indicesSize = gsl::narrow< DataIndex_t>(indices.size());

	std::optional<float> bestSplitPos =	DetailKdTree::FindSplitValue_SAH(
		currentSplitAxis, boundingBox.minBounds[currentSplitAxis.ToDim()], boundingBox.maxBounds[currentSplitAxis.ToDim()], 12.f, indices, dataElements);

	if (!bestSplitPos.has_value())
	{
		// this means that we failed to split every axis -> Abort
		if (currentSplitAxis == lastSplitAxis)
		{
			const DataIndex_t firstIndicesIndex = gsl::narrow<DataIndex_t>(m_dataIndices.size());
			m_dataIndices.insert(m_dataIndices.end(), std::begin(indices), std::end(indices));

			assert(m_dataIndices.size() == firstIndicesIndex + indicesSize);

			return KdNode::CreateLeaf(DataIndicesIndexView{ firstIndicesIndex, indicesSize });
		}
		else
		{
			// Try again with other split axis
			return CreateNodeRecursive(boundingBox, dataElements, currentSplitAxis.NextAxis(), lastSplitAxis, std::move(indices));
		}
	}

	const float splitPos = *bestSplitPos;

	DetailKdTree::SplitResult splitResult = DetailKdTree::Split(currentSplitAxis, splitPos, std::move(indices), dataElements);

	const NodePairIndex childIndexPair = m_nodeMemory.AllocNodePair();

	AABB firstBoundingBox(boundingBox);
	firstBoundingBox.maxBounds[currentSplitAxis.ToDim()] = splitPos;

	AABB secondBoundingBox(boundingBox);
	secondBoundingBox.minBounds[currentSplitAxis.ToDim()] = splitPos;



	m_nodeMemory.Access(childIndexPair.GetFirstIndex()) =
		CreateNodeRecursive(firstBoundingBox, dataElements, currentSplitAxis.NextAxis(), currentSplitAxis, std::move(splitResult.firstDataIndices));

	m_nodeMemory.Access(childIndexPair.GetSecondIndex()) =
		CreateNodeRecursive(secondBoundingBox, dataElements, currentSplitAxis.NextAxis(), currentSplitAxis, std::move(splitResult.secondDataIndices));

	return KdNode::CreateNode(childIndexPair, currentSplitAxis, splitPos);
}

inline gsl::span<const KdTree::DataIndex_t> KdTree::GetDataIndices(DataIndicesIndexView indicesView) const
{
	assert(m_dataIndices.size() >= indicesView.firstIndex.internalIndex + indicesView.size);
	return gsl::span<const DataIndex_t>(&(m_dataIndices[indicesView.firstIndex.internalIndex]), indicesView.size);
}

