#pragma once

#include "vec2.hpp"
#include "vec3.hpp"

class Camera
{
public:
	static Camera create_with_forward_dir(glm::vec3 forward_dir);

	// normalised_screen_pixel: should be in range from -1 to +1. One of the two components may be
	// greater to adjust for Aspect ratio
	glm::vec3 calc_ray_dir(glm::vec2 normalised_screen_pixel, float field_of_view_scaling) const;


	Camera(glm::vec3 forward_dir, glm::vec3 up_dir, glm::vec3 right_dir)
		: forward_dir(forward_dir), up_dir(up_dir), right_dir(right_dir) {}

	Camera() = default;
private:

	glm::vec3 forward_dir;
	glm::vec3 up_dir;
	glm::vec3 right_dir;
};
