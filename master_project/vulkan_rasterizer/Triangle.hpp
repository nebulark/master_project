#pragma once
#include "glm.hpp"
#include <optional>
#include "AABB.hpp"

struct Triangle
{
	glm::vec3 vertices[3];
	std::optional<float> RayIntersection(const glm::vec3 rayOrigin, const glm::vec3 rayDirection) const;

	static AABB CreateAABB(gsl::span<const Triangle> triangles);
};

inline std::optional<float> Triangle::RayIntersection(const glm::vec3 rayOrigin, const glm::vec3 rayDirection) const
{
	// Möller–Trumbore intersection algorithm
	// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm


	const glm::vec3 edge1 = vertices[1] - vertices[0];
	const glm::vec3 edge2 = vertices[2] - vertices[0];

	const glm::vec3 h = glm::cross(rayDirection, edge2);
	float a = glm::dot(edge1, h);


	constexpr float epsilon = 0.0000001f;
	if (std::abs(a) < epsilon)
	{
		// the ray is paralell to the triangle
		return std::nullopt;
	}

	const float f = 1.f / a;
	const glm::vec3 s = rayOrigin - vertices[0];

	const float u = f * glm::dot(s, h);
	if (u < 0.0f || u > 1.0f)
	{
		return std::nullopt;
	}

	const glm::vec3 q = glm::cross(s, edge1);
	const float v = f * glm::dot(rayDirection, q);
	if (v < 0.0f || u + v > 1.0f)
	{
		return std::nullopt;
	}

	// At this stage we can compute t to find out where the intersection point is on the line.
	const float t = f * glm::dot(edge2, q);
	return t;
}

inline AABB Triangle::CreateAABB(gsl::span<const Triangle> triangles)
{
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
	return totalBoundingBox;
}


