#pragma once
#include "glm.hpp"
#include "gtc/quaternion.hpp"

class Camera
{
public:

	glm::mat4 CalcViewMatrix() const;

	void SetPerspection(float nearPlane, float farPlane, float fieldOfViewRadians, glm::vec2 aspectRatio);
	const glm::mat4& GetProjectionMatrix() const;

	void UpdateFromMouse(float yawInput, float pitchInput);
	void UpdateLocation(float forwardInput, float leftInput);
	void LookAt(glm::vec3 pos, glm::vec3 UpDir);
	void LookDir(glm::vec3 Dir, glm::vec3 UpDir);

	glm::quat m_rotation;
	glm::vec3 m_position;
private:

	glm::mat4 m_perspectionMatrix;
};

