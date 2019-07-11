#pragma once

#include "glm.hpp"
#include "NTree.hpp"
#include "GetSizeUint32.hpp"

struct Portal
{
	int meshIndex;
	glm::mat4 a_transform;
	glm::mat4 b_transform;

	glm::mat4 a_to_b;
	glm::mat4 b_to_a;

	static Portal CreateWithPortalTransforms(int meshIndex, const glm::mat4& a_transform, const glm::mat4& b_transform);
};

inline Portal Portal::CreateWithPortalTransforms(int meshIndex, const glm::mat4& a_transform, const glm::mat4& b_transform)
{
	Portal result = { meshIndex, a_transform, b_transform };
	result.a_to_b = b_transform * glm::inverse(a_transform) ;
	result.b_to_a = glm::inverse(result.a_to_b);
	return result;
}



