#include "pch.hpp"
#include "TriangleMesh.hpp"
#include "tinyObj/tiny_obj_loader.h"
#include "KdTreeTraverser.hpp"

TriangleMesh TriangleMesh::FromFile(const char* filePath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath);
	if (!success)
	{
		throw std::runtime_error(warn + err);
	}

	std::vector<Triangle> triangles;
	for (const tinyobj::shape_t& shape : shapes)
	{
		const gsl::span<const tinyobj::index_t> meshIndices(shape.mesh.indices);
		assert(meshIndices.size() % 3 == 0);

		for (int i = 0; i < meshIndices.size(); i += 3)
		{
			gsl::span<const tinyobj::index_t, 3> triangleIndices = meshIndices.subspan(i, 3);
			constexpr size_t posSize = glm::vec3::length();

			Triangle triangle;
			for (int triangleVertexIndex = 0; triangleVertexIndex < std::size(triangle.vertices); ++triangleVertexIndex)
			{
				const int meshVertexIndex = meshIndices[i + triangleVertexIndex].vertex_index;
				triangle.vertices[triangleVertexIndex] =
				{
					attrib.vertices[posSize * meshVertexIndex + 0],
					attrib.vertices[posSize * meshVertexIndex + 1],
					attrib.vertices[posSize * meshVertexIndex + 2],
				};
			}

			triangles.push_back(triangle);
		}
	}

	return FromTriangles(std::move(triangles));
}

TriangleMesh TriangleMesh::FromTriangles(std::vector<Triangle>&& triangles)
{
	TriangleMesh triangleMesh;
	triangleMesh.m_triangles = std::move(triangles);

	triangleMesh.m_modelBoundingBox = Triangle::CreateAABB(triangleMesh.m_triangles);
	triangleMesh.m_kdtree.Init(triangleMesh.m_triangles, triangleMesh.m_modelBoundingBox);
	return triangleMesh;
}

std::optional<float> TriangleMesh::RayTrace(const Ray& ray) const
{
	const std::optional<std::array<float,2>> boundingBoxRayTrace = m_modelBoundingBox.RayTrace(ray);
	if (!boundingBoxRayTrace.has_value())
	{
		return std::nullopt;
	}

	const std::array<float, 2> & bb_rt_result = boundingBoxRayTrace.value();
	if (bb_rt_result[0] > ray.distance || bb_rt_result[1] < 0.f)
	{
		return std::nullopt;
	}

	const float tmin = std::max(bb_rt_result[0], 0.f);
	const float tmax = std::min(bb_rt_result[1], ray.distance);

	KdTreeTraverser::RayTraceData raytraceData = {};
	raytraceData.dataElements = gsl::make_span(m_triangles);
	raytraceData.ray = Ray::FromOriginAndDirection(ray.CalcPosition(tmin), ray.direction, tmax - tmin);
	raytraceData.tree = &m_kdtree;
	raytraceData.userData = nullptr;
	raytraceData.intersectionFunction = [](const Ray& ray, const Triangle& triangle, void*)
	{
		return triangle.RayIntersection(ray.origin, ray.direction);
	};

	constexpr float bigFloat = 1e10f;


	// kd tree does not work, fall back to manually tracing all triangles for now
#if 1
	constexpr float invalidBestResult = std::numeric_limits<float>::max();
	float bestresult = invalidBestResult;
	for (const Triangle& tri : m_triangles)
	{
		std::optional<float> maybeRtResult = raytraceData.intersectionFunction(raytraceData.ray, tri, nullptr);
		if (!maybeRtResult.has_value())
		{
			continue;
		}

		const float& rtResult = *maybeRtResult;
		constexpr float tolerance = 1e-7f;
		if (rtResult < -tolerance || rtResult > ray.distance + tolerance)
		{
			continue;
		}

		if (rtResult < bestresult)
		{
			bestresult = rtResult;
		}
	}

	if (bestresult != invalidBestResult)
	{
		return bestresult + tmin;
	}
	else
	{
		return std::nullopt;
	}

#else
	std::optional<KdTreeTraverser::RayTraceResult> result = KdTreeTraverser::RayTrace(raytraceData, m_kdtree.GetRootNode(), raytraceData.ray.distance);
	if (result)
	{
		return result->t + tmin;
	}
	
	return std::nullopt;
#endif
}
