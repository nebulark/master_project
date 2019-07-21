#pragma once
#include "glm.hpp"
#include "gtc/quaternion.hpp"
#include "gtx/transform.hpp"

class Transform
{
public:
	constexpr Transform()
		: translation(0.f)
		, scale(1.f)
		, rotation(glm::identity<glm::quat>()) {}

	constexpr Transform(const glm::vec3& translation, float scale, const glm::quat& rotation)
		: translation(translation)
		, scale(scale)
		, rotation(rotation) {}

	constexpr Transform(const glm::vec3& translation, const glm::vec3& scale, const glm::quat& rotation)
		: translation(translation)
		, scale(scale)
		, rotation(rotation) {}

	static Transform FromTranslation(const glm::vec3& translation) { return Transform(translation, 1.f, glm::identity<glm::quat>()); }

	glm::mat4 ToMat() const
	{
		glm::mat4 result = glm::scale(scale);
		result = glm::mat4_cast(rotation) * result;
		result = glm::translate(translation) * result;
		return result;
	}

	Transform CalcInversion() const;

	Transform& Translate(glm::vec3 inTranslation) { translation + inTranslation; return *this; }

	Transform& operator*=(const Transform& rhs);
	friend Transform operator*(const Transform& lhs, const Transform& rhs);

	static Transform CalcAtToB(const Transform& a, const Transform& b);

	glm::vec3 translation;
	glm::vec3 scale;
	glm::quat rotation;

};

inline Transform& Transform::operator*=(const Transform& rhs)
{
	translation += rhs.translation;
	scale *= rhs.scale;
	rotation = glm::normalize(rotation * rhs.rotation);
	return *this;
}

inline Transform operator*(const Transform& lhs, const Transform& rhs)
{
	Transform result(lhs);
	result *= rhs;
	return result;
}

inline Transform Transform::CalcAtToB(const Transform& a, const Transform& b)
{
	const glm::vec3 aToB_trans = b.translation - a.translation;
	const glm::vec3 aToB_scale = b.scale / a.scale;
	const glm::quat aToB_rot = glm::normalize(b.rotation * glm::inverse(a.rotation));
	return Transform(aToB_trans, aToB_scale, aToB_rot);
}

inline Transform Transform::CalcInversion() const
{
	return Transform(
		-translation,
		1.f / scale,
		glm::normalize(glm::inverse(rotation))
	);
}

