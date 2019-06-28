#pragma once
#include "glm.hpp"
#include "gtc/quaternion.hpp"
#include "Transform.hpp"

class Camera
{
public:

	void SetPerspection(float nearPlane, float farPlane, float fieldOfViewRadians, glm::vec2 aspectRatio);
	const glm::mat4& GetProjectionMatrix() const;

	void UpdateFromMouse(float yawInput, float pitchInput);
	void UpdateLocation(float forwardInput, float rightInput, float upInput);
	void LookAt(glm::vec3 pos, glm::vec3 upDir);
	void LookDir(glm::vec3 dir, glm::vec3 upDir);

	Transform m_transform;
private:

	glm::mat4 m_perspectionMatrix;
};

