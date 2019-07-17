#pragma once

#include <glm.hpp>
#include <optional>
#include "Ray.hpp"

struct AABB
{
	glm::vec3 minBounds;
	glm::vec3 maxBounds;

	// expands AABB so that it can contain the point
	constexpr void ExpandToContain(const glm::vec3& point)
	{
		for (int dimIdx = 0; dimIdx < glm::vec3::length(); dimIdx++) {
			const float dimVal = point[dimIdx];
			if (dimVal< minBounds[dimIdx]) {
				 minBounds[dimIdx] = dimVal;
			}
			if (dimVal > maxBounds[dimIdx]) {
				maxBounds[dimIdx] = dimVal;
			}
		}
	}

	constexpr bool Contains(const glm::vec3& point) const
	{
		for (int dimIdx = 0; dimIdx < glm::vec3::length(); dimIdx++) {
			const float dimVal = point[dimIdx];
			if (dimVal< minBounds[dimIdx]) {
				return false;
			}
			if (dimVal > maxBounds[dimIdx]) {
				return false;
			}
		}

		return true;
	}

	constexpr int FindWidestDim() const
	{
		const glm::vec3 width = maxBounds - minBounds;

		int widetstDim = 0;
		float widestDimVal = width[0];

		for (int i = 1; i < glm::vec3::length(); ++i)
		{
			if (widestDimVal < width[i])
			{
				widestDimVal = width[i];
				widetstDim = i;
			}
		}

		return widetstDim;
	}

	constexpr std::optional<std::array<float,2>> RayTrace(const Ray& ray) const
	{
		// https://tavianator.com/fast-branchless-raybounding-box-intersections/
		// slab method

		float tmin = std::numeric_limits<float>::lowest();
		float tmax = std::numeric_limits<float>::max();

		{

			// find tmin and tmax only considering x
			const float tx1 = (minBounds.x - ray.origin.x) * ray.inverseDirection.x;
			const float tx2 = (maxBounds.x - ray.origin.x) * ray.inverseDirection.x;

			const float tminx = std::min(tx1, tx2);
			const float tmaxx = std::max(tx1, tx2);
			tmin = std::max(tmin, tminx);
			tmax = std::min(tmax, tmaxx);
		}


		// find tminy and tmaxy only considering y
		{
			const float ty1 = (minBounds.y - ray.origin.y) * ray.inverseDirection.y;
			const float ty2 = (maxBounds.y - ray.origin.y) * ray.inverseDirection.y;

			const float tminy = std::min(ty1, ty2);
			const float tmaxy = std::max(ty1, ty2);

			// combine tminy and tmaxy with previous
			// tminy can only increase tmin
			// while tmaxy can only decrease tmin
			tmin = std::max(tmin, tminy);
			tmax = std::min(tmax, tmaxy);
		}
		// same with z
		{
			const float tz1 = (minBounds.z - ray.origin.z) * ray.inverseDirection.z;
			const float tz2 = (maxBounds.z - ray.origin.z) * ray.inverseDirection.z;

			const float tminz = std::min(tz1, tz2);
			const float tmaxz = std::max(tz1, tz2);

			// combine tminz and tmaxz with previous
			// tminz can only increase tmin
			// while tmaxz can only decrease tmin
			tmin = std::max(tmin, tminz);
			tmax = std::min(tmax, tmaxz);
		}

		if (tmax < tmin)
		{
			return std::nullopt;
		}

		return std::array<float,2>{ tmin, tmax };
	}
};

constexpr AABB InvalidAABB = AABB{ glm::vec3(std::numeric_limits<float>::max()), glm::vec3(std::numeric_limits<float>::lowest()) };

constexpr bool operator==(const AABB& lhs, const AABB& rhs) noexcept
{
	return lhs.minBounds == rhs.minBounds
		&& lhs.maxBounds == rhs.maxBounds
		;
}

constexpr bool operator!=(const AABB& lhs, const AABB& rhs) noexcept
{
	return !(lhs == rhs);
}


struct AABBEdgePoints
{
	std::array<glm::vec3, 4> mainVertices;
	std::array<glm::vec3, 4> supportVertices;
};

inline constexpr AABBEdgePoints CreateEdgePoints(const AABB& aabb)
{
	auto createVertices = [](glm::vec3 firstVertex, glm::vec3  secondVertex)
	{
		std::array<glm::vec3, 4> result =
		{
			glm::vec3(firstVertex.x, firstVertex.y, firstVertex.z),
			glm::vec3(firstVertex.x, secondVertex.y, secondVertex.z),
			glm::vec3(secondVertex.x, firstVertex.y, secondVertex.z),
			glm::vec3(secondVertex.x, secondVertex.y, firstVertex.z),
		};
		return result;
	};

	return AABBEdgePoints
	{
		createVertices(aabb.minBounds, aabb.maxBounds),
		createVertices(aabb.maxBounds, aabb.minBounds),
	};
}

inline void ApplyMatrix(AABBEdgePoints& edgePoints, const glm::mat4& matrix)
{
	assert(std::size(edgePoints.mainVertices) == std::size(edgePoints.supportVertices));
	for (int i = 0; i < std::size(edgePoints.mainVertices); ++i)
	{
		edgePoints.mainVertices[i] = glm::vec3(matrix * glm::vec4(edgePoints.mainVertices[i], 1.f));
		edgePoints.supportVertices[i] = glm::vec3(matrix * glm::vec4(edgePoints.supportVertices[i], 1.f));
	}
}

inline std::array<glm::vec3, 24> CreateEdgeLines(const AABBEdgePoints& edgePoints)
{
	constexpr int lineCount = 12;

	std::array<glm::vec3, lineCount * 2> result;
	int resultIdx = 0;

	for (int mainIdx = 0; mainIdx < std::size(edgePoints.mainVertices); ++mainIdx)
	{
		const glm::vec3 mainVertex = edgePoints.mainVertices[mainIdx];

		for (
			int supportIdx = (mainIdx + 1) % std::size(edgePoints.supportVertices);
			supportIdx != mainIdx;
			supportIdx = (supportIdx + 1) % std::size(edgePoints.supportVertices)
			)
		{
			const glm::vec3 supportVertex = edgePoints.supportVertices[supportIdx];
			result[resultIdx++] = mainVertex;
			result[resultIdx++] = supportVertex;
		}
	}

	assert(std::size(result) == resultIdx);
	return result;
}

