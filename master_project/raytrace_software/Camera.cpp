#include "pch.h"
#include "Camera.hpp"

Camera Camera::create_with_forward_dir(glm::vec3 in_forward_dir)
{
	const glm::vec3 forward_dir = glm::normalize(in_forward_dir);
	constexpr glm::vec3 world_up{ 0.f, 1.f, 0.f };
	const glm::vec3  right_dir = glm::cross(forward_dir, world_up);
	const glm::vec3 up_dir = glm::cross(right_dir, forward_dir);

	return Camera{ forward_dir, up_dir, right_dir };
}

glm::vec3 Camera::calc_ray_dir(glm::vec2 normalised_screen_pixel, float field_of_view_scaling) const
{

	// we treat camera origin as 0, we are only interested in direction so it does not matter
	// if the two points whicht define that direction are translated by the same amount

	// We can treat the foward vector as the origin of the cameras projection plane beeing one unit away from the camera
	// while using up and right as the vectors to definde the plane

	const glm::vec3 point_on_projection_plane = forward_dir
		+ (normalised_screen_pixel.x * field_of_view_scaling) * right_dir
		+ (normalised_screen_pixel.y * field_of_view_scaling) * up_dir;
	
	// we can just use the normalized point as the origin is 0.
	return glm::normalize(point_on_projection_plane);
}
