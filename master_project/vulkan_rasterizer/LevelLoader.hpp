#pragma once

#include "Transform.hpp"

namespace LevelLoader
{
	struct LevelObject
	{
		int meshId;
		std::string name;
		Transform transform;
	};

	struct PortalObject
	{
		int meshId;
		std::string name;
		Transform transformA;
		Transform transformB;
	};

	struct CameraObject
	{
		glm::vec3 positon;
		glm::vec3 target;
	};

	struct LoadLevelResult
	{
		std::vector<std::string> objFileNames;
		std::vector<LevelObject> objects;
		std::vector<PortalObject> portals;
		std::vector<CameraObject> cameras;
	};

	LoadLevelResult LoadLevel(const char* fileName);


}
