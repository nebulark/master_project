#pragma once
#include "glm.hpp"
struct Ray
{
	static constexpr Ray FromOriginAndDirection(const glm::vec3& origin, const glm::vec3& direction, const float distance = 1e10f)
	{
		return Ray{ origin,direction, 1.f / direction, distance };
	}

	static Ray FromStartAndEndpoint(const glm::vec3& start, const glm::vec3& end)
	{
		const glm::vec3 delta = start - end;

		const float distance = std::sqrt(glm::dot(delta, delta));
		const glm::vec3 dir = delta / distance;

		return Ray::FromOriginAndDirection(start,dir,distance);
	}

	glm::vec3 origin;
	glm::vec3 direction;
	glm::vec3 inverseDirection;
	float distance;
};
