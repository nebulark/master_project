#pragma once
#include "Triangle.hpp"
#include <vector>
#include "AABB.hpp"
#include "KdTree.hpp"
#include <optional>

struct Ray;

class TriangleMesh
{
public:
	static TriangleMesh FromFile(const char* filePath);
	static TriangleMesh FromTriangles(std::vector<Triangle>&& triangles);

	// transforms ray into modelspace and performs intersection
	std::optional<float> RayTrace(const glm::vec3 rayOrigin, const glm::vec3 rayDir, const glm::mat4& inverseModelMatrix);
private:
	std::vector<Triangle> m_triangles;
	AABB m_modelBoundingBox;
	KdTree m_kdtree;
};
