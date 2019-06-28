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

	constexpr Transform(glm::vec3 translation, float scale, glm::quat rotation)
		: translation(translation)
		, scale(scale)
		, rotation(rotation) {}

	constexpr Transform(glm::vec3 translation, glm::vec3 scale, glm::quat rotation)
		: translation(translation)
		, scale(scale)
		, rotation(rotation) {}

	glm::mat4 ToMat() const
	{
		glm::mat4 result = glm::scale(scale);
		result = glm::mat4_cast(rotation) * result;
		result = glm::translate(translation) * result;
		return result;
	}

	glm::mat4 ToViewMat() const
	{
		return glm::inverse(ToMat());
	}

	Transform& Translate(glm::vec3 inTranslation) { translation + inTranslation; return *this; }

	Transform& operator*=(const Transform& rhs);
	friend Transform operator*(const Transform& lhs, const Transform& rhs);

	glm::vec3 translation;
	glm::vec3 scale;
	glm::quat rotation;

};

inline Transform& Transform::operator*=(const Transform& rhs)
{
	translation += rhs.translation;
	scale *= rhs.scale;
	rotation *= rhs.rotation;
}

inline Transform operator*(const Transform& lhs, const Transform& rhs)
{
	Transform result(lhs);
	result *= rhs;
	return lhs;
}



