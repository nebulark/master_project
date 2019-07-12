#pragma once

#include <glm.hpp>
#include "SplitAxis.hpp"

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
};


