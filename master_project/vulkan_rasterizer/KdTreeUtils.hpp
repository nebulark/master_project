#pragma once

#include <cstdint>
#include <vector>
#include <cassert>
#include "SplitAxis.hpp"

struct NodeIndex
{
	uint32_t internalIndex;
};

struct NodePairIndex
{
	NodeIndex internalLeftIndex;
	constexpr NodeIndex GetLeftIndex() const { return  internalLeftIndex; };
	constexpr NodeIndex GetRightIndex() const { return NodeIndex{ internalLeftIndex.internalIndex + 1 }; };
};

struct DataIndicesIndex
{
	uint32_t internalIndex;
};

struct DataIndicesIndexView
{
	DataIndicesIndex firstIndex;
	uint32_t size;
};

class KdNode
{
public:
	SplitAxis GetSplitAxis() const noexcept;
	NodePairIndex GetChildNodeIndexPair() const noexcept;
	DataIndicesIndexView GetLeaveIndexView() const noexcept;
	const float GetSplitVal() const noexcept;

	static KdNode CreateNode(NodePairIndex childNodes, SplitAxis splitAxis, float splitVal);
	static KdNode CreateLeaf(DataIndicesIndexView dataIndexView);

private:
	uint32_t GetIndex() const noexcept;

	// split axis, and either DataIndex or NodeIndex depending on split axis value
	uint32_t indexAndSplitAxis;

	static constexpr int splitAxisBits = 2;
	static_assert((1 << splitAxisBits) <= static_cast<int>(SplitAxis::enum_size));

	static constexpr int splitAxisRightShifts = sizeof(uint32_t) * CHAR_BIT - splitAxisBits;
	static constexpr uint32_t splitAxisMask = (~uint32_t(0)) << splitAxisRightShifts;
	static constexpr uint32_t indexMask = ~splitAxisMask;

	union FloatOrUInt32
	{
		float splitVal;
		uint32_t dataViewSize;
	};

	FloatOrUInt32 splitValOrDataViewSize;
};

static_assert(sizeof(KdNode) == sizeof(uint32_t) * 2);

inline SplitAxis KdNode::GetSplitAxis() const {
	uint32_t splitAxisVal = (indexAndSplitAxis >> splitAxisRightShifts);
	assert(splitAxisVal < static_cast<uint32_t>(SplitAxis::enum_size));
	return static_cast<SplitAxis>(splitAxisVal);
};

inline uint32_t KdNode::GetIndex() const noexcept
{
	return indexAndSplitAxis & indexMask;
}

inline NodePairIndex KdNode::GetChildNodeIndexPair() const noexcept
{
	assert(GetSplitAxis() != SplitAxis::leafNode);
	return NodePairIndex{ GetIndex() };
}

inline const float KdNode::GetSplitVal() const noexcept
{

	assert(GetSplitAxis() != SplitAxis::leafNode);
	return splitValOrDataViewSize.splitVal;
}

inline DataIndicesIndexView KdNode::GetLeaveIndexView() const noexcept
{
	assert(GetSplitAxis() == SplitAxis::leafNode);
	const uint32_t index = GetIndex();
	const uint32_t size = splitValOrDataViewSize.dataViewSize;

	return DataIndicesIndexView{ index, size };
}

inline KdNode KdNode::CreateNode(NodePairIndex childNodes, SplitAxis splitAxis, float splitVal)
{
	const uint32_t splitAxis_shifted = (static_cast<uint32_t>(SplitAxis::leafNode)) << splitAxisRightShifts;

	KdNode result;
	result.indexAndSplitAxis = childNodes.internalLeftIndex.internalIndex | splitAxis_shifted;
	result.splitValOrDataViewSize.splitVal = splitVal;
	return result;
}

inline KdNode KdNode::CreateLeaf(DataIndicesIndexView dataIndexView)
{
	constexpr uint32_t leafNode = (static_cast<uint32_t>(SplitAxis::leafNode)) << splitAxisRightShifts;
	static_assert((leafNode & splitAxisMask) != 0);

	assert((dataIndexView.firstIndex.internalIndex & splitAxisMask) == 0 && "data index view index too large");

	KdNode result;
	result.indexAndSplitAxis = dataIndexView.firstIndex.internalIndex | leafNode;
	result.splitValOrDataViewSize.dataViewSize = dataIndexView.size;
	return result;
}



// storage for our KdTree, we can optimize this later
class KdNodeMemory
{
public:
	NodePairIndex AllocNodePair();

	KdNode& Access(NodeIndex index) { return m_nodes[index.internalIndex]; }
	const KdNode& Get(NodeIndex index) const { return m_nodes[index.internalIndex]; }


private:
	std::vector<KdNode> m_nodes;
};


inline NodePairIndex KdNodeMemory::AllocNodePair()
{
	int internalLeftIndex = static_cast<int>(m_nodes.size());

	// add two nodes
	m_nodes.emplace_back();
	m_nodes.emplace_back();

	return NodePairIndex{ internalLeftIndex };
}


