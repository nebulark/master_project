#include "pch.hpp"
#include "Camera.hpp"

glm::mat4 Camera::CalcViewMatrix() const
{
	glm::mat4 camera{1};
	camera = glm::translate(camera, m_position);
	camera = camera * glm::mat4_cast(m_rotation);
	return glm::inverse(camera);
}

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
	glm::quat yawRotation = glm::angleAxis(yawInput, glm::vec3(0, -1, 0));
	glm::quat pitchRotation = glm::angleAxis(pitchInput, glm::vec3(-1, 0, 0));
	m_rotation = glm::normalize(yawRotation * m_rotation * pitchRotation);
}

void Camera::UpdateLocation(float forwardInput, float rightInput, float upInput)
{
	glm::vec3 forwardVector = m_rotation * glm::vec3(0, 0, -1);
	glm::vec3 upVector = m_rotation * glm::vec3(0, 1, 0);
	glm::vec3 rightVector = m_rotation * glm::vec3(1, 0, 0);

	m_position += forwardInput * forwardVector;
	m_position += upInput * upVector;
	m_position += rightInput * rightVector;
}

void Camera::LookAt(glm::vec3 pos, glm::vec3 upDir)
{
	LookDir(pos - m_position, upDir);
}

void Camera::LookDir(glm::vec3 dir, glm::vec3 upDir)
{
	m_rotation = glm::quatLookAt(dir, upDir);
}
