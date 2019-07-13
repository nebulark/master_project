#pragma once

#include <glm.hpp>
#include "SplitAxis.hpp"
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

	constexpr SplitAxis FindWidestDim() const
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

		assert(widetstDim <= static_cast<int>(SplitAxis::dim_z));
		return static_cast<SplitAxis>(widetstDim);
	}

	constexpr std::optional<float> RayTrace(const Ray& ray) const
	{
		// https://tavianator.com/fast-branchless-raybounding-box-intersections/
		// slab method


		// find tmin and tmax only considering x
		const float tx1 = (minBounds.x - ray.origin.x) * ray.inverseDirection.x;
		const float tx2 = (minBounds.x - ray.origin.x) * ray.inverseDirection.x;

		float tmin = std::min(tx1, tx2);
		float tmax = std::max(tx1, tx2);
		


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

		return tmax >= tmin;
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
