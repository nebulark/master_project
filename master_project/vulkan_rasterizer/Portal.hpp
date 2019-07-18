#pragma once

#include "glm.hpp"
#include "NTree.hpp"
#include "GetSizeUint32.hpp"
#include "EnumIndex.hpp"



enum class PortalEndpoint
{
	A,
	B,
};

using PortalEndpointIndex = EnumIndex<PortalEndpoint, PortalEndpoint::A, PortalEndpoint::B>;







struct Portal
{
	int meshIndex;
	std::array<glm::mat4, PortalEndpointIndex::GetRange()> transform;
	std::array<glm::mat4, PortalEndpointIndex::GetRange()> toOtherEndpoint;

	static Portal CreateWithPortalTransforms(int meshIndex, const glm::mat4& a_transform, const glm::mat4& b_transform);
};

inline Portal Portal::CreateWithPortalTransforms(int meshIndex, const glm::mat4& a_transform, const glm::mat4& b_transform)
{
	Portal result;

	result.meshIndex = meshIndex;
	result.transform = { a_transform, b_transform };

	const glm::mat4 a_to_b = b_transform * glm::inverse(a_transform);
	const glm::mat4 b_to_a = glm::inverse(a_to_b);

	result.toOtherEndpoint = { a_to_b, b_to_a };
	return result;
}



