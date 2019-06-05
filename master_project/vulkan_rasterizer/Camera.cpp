#include "pch.hpp"
#include "Camera.hpp"

glm::mat4 Camera::CalcViewMatrix() const
{
	glm::mat4 view = glm::lookAt(glm::vec3(0.f), glm::vec3(-2.0f, -2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	//return glm::translate(view, -glm::vec3(2.f, 2.f, 0.f));


	glm::mat4 result = view;
	return glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(-2.0f, -2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));


#if 0
	glm::mat3 rotationMat = glm::mat3_cast(glm::quatLookAt(-glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
		//glm::mat3_cast(m_rotation);
	glm::vec3 forwardVector = rotationMat * glm::vec3(0, 0, 1);
	glm::vec3 upVector = rotationMat * glm::vec3(0, 1, 0);

//	return glm::lookAtRH(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat	bla = glm::toQuat(glm::lookAtRH(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

//	return glm::translate(glm::mat4_cast(glm::normalize(glm::quatLookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 1.0f)))), -m_position);


	//return glm::mat4_cast(bla);

	//return glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));







		return glm::lookAt(m_position, m_position + forwardVector, upVector);
#endif
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
	glm::quat yawRotation = glm::angleAxis(yawInput, glm::vec3(0, 1, 0));
	glm::quat pitchRotation = glm::angleAxis(pitchInput, glm::vec3(1, 0, 0));
	m_rotation = glm::normalize(yawRotation * m_rotation * pitchRotation);
}

void Camera::UpdateLocation(float forwardInput, float leftInput)
{
	glm::vec3 forwardVector = m_rotation * glm::vec3(0, 0, 1);
	glm::vec3 rightVector = m_rotation * glm::vec3(1, 0, 0);

	m_position += forwardInput * forwardVector;
	m_position += leftInput * rightVector;
}

void Camera::LookAt(glm::vec3 pos, glm::vec3 UpDir)
{
	m_rotation = glm::toQuat(glm::lookAt(m_position, pos, UpDir));
}

void Camera::LookDir(glm::vec3 Dir, glm::vec3 UpDir)
{
	LookAt(m_position + Dir, UpDir);
}
