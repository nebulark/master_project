#pragma once

namespace LevelLoader
{
	struct LevelObject
	{
		int meshId;
		std::string name;
		glm::mat4 mat;
	};

	struct LoadLevelResult
	{
		std::vector<std::string> objFileNames;
		std::vector<LevelObject> objects;

	};

	LoadLevelResult LoadLevel(const char* fileName);


}
