#pragma once
#include "glm.hpp"
#include "KdTreeUtils.hpp"
#include <vector>
#include <gsl/gsl>
#include "AABB.hpp"

struct Triangle
{
	glm::vec3 vertices[3];
};

struct DataIndex
{
	int internalIndex;
};

class KdTree
{
public:
	static constexpr int MaxIndicesPerNode = 8;

	void Init(gsl::span<Triangle> triangles);

	const Triangle& GetDataElement(DataIndex index) const { return m_data[index.internalIndex]; }
private:

	void InitRescursion(KdNode& inOutnode, const AABB& boundingBox, std::vector<DataIndex>&& indices);

	KdNode m_firstNode;
	KdNodeMemory m_nodeMemory;
	std::vector<Triangle> m_data;
	std::vector<DataIndex> m_dataIndices;
};

namespace KdTreeDetail
{

	struct SplitResult
	{
		std::vector<DataIndex> first;
		std::vector<DataIndex> second;
		float splitValue;
	};

	SplitResult Split(SplitAxis axis, std::vector<DataIndex>&& elementIndices, const KdTree& tree)
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

		std::nth_element(begin, median, end, [tree, &GetCentroid](DataIndex a, DataIndex b)
			{
				const Triangle& tri_a = tree.GetDataElement(a);
				const Triangle& tri_b = tree.GetDataElement(b);
				return GetCentroid(tri_a) < GetCentroid(tri_b);
			});

		const Triangle& medianTri = tree.GetDataElement(*median);

		// using maximum of tri vertices to ensure the triangle is in the first half
		const float splitValue = std::max({ medianTri.vertices[0][dim], medianTri.vertices[1][dim], medianTri.vertices[2][dim] });

		SplitResult result;

		result.first.reserve(elementIndices.size());
		result.second.reserve(elementIndices.size());
		result.splitValue = splitValue;

		for (DataIndex elementIndex : elementIndices)
		{
			const Triangle& tri = tree.GetDataElement(elementIndex);


			if (   tri.vertices[0][dim] < splitValue
				|| tri.vertices[1][dim] < splitValue
				|| tri.vertices[2][dim] < splitValue)
			{
				result.first.push_back(elementIndex);
			}

			if (   tri.vertices[0][dim] > splitValue
				|| tri.vertices[1][dim] > splitValue
				|| tri.vertices[2][dim] > splitValue)
			{
				result.second.push_back(elementIndex);
			}
		}

		return result;
	}
}

inline void KdTree::Init(gsl::span<Triangle> triangles)
{
	const uint32_t elementCount = triangles.size();

	// special case 0 elements
	if (elementCount == 0)
	{
		m_firstNode = KdNode::CreateLeaf(DataIndicesIndexView{ 0, 0 });
		return;
	}

	m_data.reserve(elementCount);
	m_data.insert(m_data.end(),  triangles.begin(), triangles.end());

	// special case, to few elements to create "real" kdtree
	if (elementCount <= MaxIndicesPerNode)
	{
		m_dataIndices.resize(elementCount);

		for (int i = 0; i < m_dataIndices.size(); ++i)
		{
			m_dataIndices[i].internalIndex = i;
		}

		m_firstNode = KdNode::CreateLeaf(DataIndicesIndexView{ 0, elementCount });
		assert(m_dataIndices.size() == m_data.size());

		return;
	}

	AABB totalBoundingBox;
	totalBoundingBox.minBounds = triangles[0].vertices[0];
	totalBoundingBox.maxBounds = triangles[0].vertices[0];

	for (const Triangle& tri : triangles)
	{
		for (const glm::vec3& vertex : tri.vertices)
		{
			totalBoundingBox.ExpandToContain(vertex);
		}
	}

	std::vector<DataIndex> initialDataIndices;
	initialDataIndices.resize(elementCount);
	for (int i = 0; i < elementCount; ++i)
	{
		initialDataIndices[i].internalIndex = i;
	}


	InitRescursion(m_firstNode, totalBoundingBox, std::move(initialDataIndices));
}

inline void KdTree::InitRescursion(KdNode& inOutnode, const AABB& boundingBox, std::vector<DataIndex>&& indices)
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

	KdTreeDetail::SplitResult splitResult = KdTreeDetail::Split(widestDim, std::move(indices), *this);

	const NodePairIndex childIndexPair = m_nodeMemory.AllocNodePair();

	inOutnode = KdNode::CreateNode(childIndexPair, widestDim, splitResult.splitValue);

	AABB firstBoundingBox(boundingBox);
	firstBoundingBox.maxBounds[static_cast<int>(widestDim)] = splitResult.splitValue;

	AABB secondBoundingBox(boundingBox);
	secondBoundingBox.minBounds[static_cast<int>(widestDim)] = splitResult.splitValue;

	InitRescursion(m_nodeMemory.Access(childIndexPair.GetLeftIndex()), firstBoundingBox, std::move(splitResult.first));
	InitRescursion(m_nodeMemory.Access(childIndexPair.GetRightIndex()), secondBoundingBox, std::move(splitResult.second));
}

