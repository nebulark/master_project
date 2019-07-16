#pragma once
#include <array>

namespace MeshDefinitions
{
	enum MeshIds {
		torusIdx = 0,
		sphereIdx,
		cubeIdx,
		planeIdx,
		halfSphereIdx,
		invertedCubeIdx,

		enum_size,
	};

	constexpr std::array<const char*, MeshIds::enum_size> MeshObjFiles =
	{
		"torus.obj", "sphere.obj" ,"cube.obj", "plane.obj", "halfSphere.obj", "inverted_cube.obj"
	};
};
