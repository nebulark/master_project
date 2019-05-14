#pragma once

#include <optional>

#include "Ray.hpp"
#include "Sphere.hpp"
#include "glm.hpp"

namespace Intersection
{
	std::optional<float> ray_sphere(Ray ray, Sphere sphere)
	{
		const glm::vec3 oc = ray.origin - sphere.positon;
		const float a =  glm::dot(ray.direction, ray.direction);
		const float b = 2.f * glm::dot(oc, ray.direction);
		const float c = glm::dot(oc, oc) - sphere.radius * sphere.radius;
		const float discriminant = b * b - 4.f * a * c;
		if (discriminant < 0) {
			return std::nullopt;
		}
		else {
			return (-b - std::sqrt(discriminant)) / (2.f * a);
		}
	}
}