#include "pch.hpp"
#include "Camera.hpp"

void Camera::SetPerspection(float nearPlane, float farPlane, float fieldOfView, glm::vec2 aspectRatio)
{
	m_perspectionMatrix = glm::perspectiveFov(fieldOfView, aspectRatio.x, aspectRatio.y, nearPlane, farPlane);

	// Need to flip Y, it is different in Vulkan compared to OpenGL
	m_perspectionMatrix[1][1] *= -1;
}

const glm::mat4& Camera::GetProjectionMatrix() const
{
	return m_perspectionMatrix;
}

void Camera::UpdateFromMouse(float yawInput, float pitchInput)
{
	const glm::quat yawRotation = glm::angleAxis(yawInput, glm::vec3(0, -1, 0));
	const glm::quat pitchRotation = glm::angleAxis(pitchInput, glm::vec3(-1, 0, 0));

	m_transform.rotation = glm::normalize(yawRotation * m_transform.rotation * pitchRotation);
}

void Camera::UpdateLocation(float forwardInput, float rightInput, float upInput)
{
	const glm::vec3 forwardVector = m_transform.rotation * glm::vec3(0, 0, -1);
	const glm::vec3 upVector = m_transform.rotation * glm::vec3(0, 1, 0);
	const glm::vec3 rightVector = m_transform.rotation * glm::vec3(1, 0, 0);

	m_transform.translation += forwardInput * forwardVector;
	m_transform.translation += upInput * upVector;
	m_transform.translation += rightInput * rightVector;
}

void Camera::LookAt(glm::vec3 pos, glm::vec3 upDir)
{
	LookDir(pos - m_transform.translation, upDir);
}

void Camera::LookDir(glm::vec3 dir, glm::vec3 upDir)
{
	m_transform.rotation = glm::quatLookAt(dir, upDir);
}
