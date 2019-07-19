#pragma once
#include "glm.hpp"
#include "gtc/quaternion.hpp"
#include "Transform.hpp"


class Camera
{
public:
	Camera();
	void SetPerspection(float nearPlane, float farPlane, float fieldOfViewRadians, glm::vec2 aspectRatio);
	const glm::mat4& GetProjectionMatrix() const;

	void UpdateFromMouse(float yawInput, float pitchInput);
	glm::vec3 CalcForwardVector() const;
	glm::vec3 CalcUpVector() const;
	glm::vec3 CalcRightVector() const;
	void UpdateLocation(float forwardInput, float rightInput, float upInput);
	void LookAt(glm::vec3 pos, glm::vec3 upDir);
	void LookDir(glm::vec3 dir, glm::vec3 upDir);

	glm::mat4 m_coordinateSystem;

	glm::mat4 CalcMat() const;
	glm::vec3 CalcPosition() const;
	void SetPosition(const glm::vec3 pos);
	float GetNearClipping() const { return m_nearClipping; };
	float GetFarClipping() const { return m_farClipping; }
private:

	
	float m_nearClipping;
	float m_farClipping;
	Transform m_transform;
	glm::mat4 m_perspectionMatrix;
};

