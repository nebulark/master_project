#pragma once
#include "glm.hpp"
struct Ray
{
	static constexpr Ray FromOriginAndDirection(const glm::vec3& origin, const glm::vec3& direction)
	{
		return Ray{ origin,direction, 1.f / direction };
	}

	glm::vec3 origin;
	glm::vec3 direction;
	glm::vec3 inverseDirection;
};
