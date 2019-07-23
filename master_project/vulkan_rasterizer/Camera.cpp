#include "pch.hpp"
#include "Camera.hpp"


Camera::Camera()
{
	m_coordinateSystem = glm::mat4(1.f);
}

void Camera::SetPerspection(float nearPlane, float farPlane, float fieldOfView, glm::vec2 aspectRatio)
{
	m_nearClipping = nearPlane;
	m_farClipping = farPlane;
	m_perspectionMatrix = glm::perspectiveFov(fieldOfView, aspectRatio.x, aspectRatio.y, nearPlane, farPlane);

	// Need to flip Y, it is different in Vulkan compared to OpenGL
	m_perspectionMatrix[1][1] *= -1;
}


void Camera::SetPerspection_InverseZBuffer(float nearPlane, float fieldOfViewRadians, glm::vec2 aspectRatio)
{
	m_nearClipping = nearPlane;
	m_farClipping = std::numeric_limits<float>::infinity();

	float h = glm::cos(0.5f * fieldOfViewRadians) / glm::sin(0.5f * fieldOfViewRadians);
	float w = h * aspectRatio.y / aspectRatio.x;

	m_perspectionMatrix  = glm::mat4(
		w,		0.0f,	0.0f,		0.0f,
		0.0f,	-h,		0.0f,		0.0f,
		0.0f,	0.0f,	0.0f,		-1.0f,
		0.0f,	0.0f,	nearPlane,	0.0f);

}

const glm::mat4& Camera::GetProjectionMatrix() const
{
	return m_perspectionMatrix;
}

void Camera::UpdateFromMouse(float yawInput, float pitchInput)
{
	const glm::quat yawRotation = glm::angleAxis(-yawInput, glm::vec3(0, 1, 0));
	const glm::quat pitchRotation = glm::angleAxis(-pitchInput, glm::vec3(1, 0, 0));

	m_transform.rotation = glm::normalize(yawRotation * m_transform.rotation * pitchRotation);
}

glm::vec3 Camera::CalcForwardVector() const 
{
	return m_transform.rotation * glm::vec3(0, 0, -1);
}
glm::vec3 Camera::CalcUpVector() const 
{
	return m_transform.rotation * glm::vec3(0, 1, 0);
}
glm::vec3 Camera::CalcRightVector() const 
{
	return m_transform.rotation * glm::vec3(1, 0, 0);
}



void Camera::UpdateLocation(float forwardInput, float rightInput, float upInput)
{
	m_transform.translation += forwardInput * CalcForwardVector();
	m_transform.translation += upInput * CalcUpVector();
	m_transform.translation += rightInput * CalcRightVector();
}

void Camera::LookAt(glm::vec3 pos, glm::vec3 upDir)
{
	LookDir(pos - m_transform.translation, upDir);
}

void Camera::LookDir(glm::vec3 dir, glm::vec3 upDir)
{
	m_transform.rotation = glm::quatLookAt(dir, upDir);
}

glm::mat4 Camera::CalcMat() const
{
	return m_coordinateSystem * m_transform.ToMat();
}

glm::vec3 Camera::CalcPosition() const
{
	return glm::vec3(m_coordinateSystem * glm::vec4(m_transform.translation, 1.f));
}

void Camera::SetPosition(const glm::vec3 pos)
{
	m_transform.translation =glm::vec3( glm::inverse(m_coordinateSystem) * glm::vec4(pos,1.f));
}
