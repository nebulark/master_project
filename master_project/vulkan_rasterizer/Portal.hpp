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

struct TranslationPortal
{
	int meshIndex;
	std::array<glm::mat4, PortalEndpointIndex::GetRange()> transform;
	std::array<glm::mat4, PortalEndpointIndex::GetRange()> toOtherEndpoint;

	static TranslationPortal CreateWithPortalTransforms(int meshIndex, const glm::mat4& a_transform, const glm::mat4& b_transform);
};


inline TranslationPortal TranslationPortal::CreateWithPortalTransforms(int meshIndex, const glm::mat4& a_transform, const glm::mat4& b_transform)
{
	TranslationPortal result;

	result.meshIndex = meshIndex;
	result.transform = { a_transform, b_transform };

	const glm::mat4 a_to_b = b_transform * glm::inverse(a_transform);
	const glm::mat4 b_to_a = glm::inverse(a_to_b);

	result.toOtherEndpoint = { a_to_b, b_to_a };
	return result;
}


enum class PortalSide
{
	outside,
	inside,
};

using PortalSideIndex = EnumIndex<PortalSide, PortalSide::outside,  PortalSide::inside>;

struct DistortionPortal
{
	int meshIndex;
	glm::mat4 transform;
	std::array<glm::mat4, PortalSideIndex::GetRange()> distortionMatrix;

	static DistortionPortal CreateWithTransformAndOutsideDistortion(int meshIndex, const glm::mat4& transform, const glm::mat4& outsideDistortion);
};

inline DistortionPortal DistortionPortal::CreateWithTransformAndOutsideDistortion(
	int meshIndex, const glm::mat4& transform, const glm::mat4& outsideDistortion)
{
	const glm::mat4 insideDistortion = glm::inverse(outsideDistortion);
	return DistortionPortal{ meshIndex, transform, {outsideDistortion, insideDistortion} };
}

