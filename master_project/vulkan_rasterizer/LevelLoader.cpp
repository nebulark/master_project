#include "pch.hpp"
#include "LevelLoader.hpp"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"

namespace
{

	constexpr std::string_view rootNodeName("root");

	constexpr std::string_view meshesNodeName("meshes");
	constexpr std::string_view fileNodeName("file");
	constexpr std::string_view sceneNodeName("scene");
	constexpr std::string_view objectNodeName("object");
	constexpr std::string_view meshindexNodeName("meshindex");
	constexpr std::string_view nameNodeName("name");
	constexpr std::string_view transformNodeName("transform");
}

LevelLoader::LoadLevelResult LevelLoader::LoadLevel(const char* fileName)
{

	LoadLevelResult result;

	rapidxml::file<> file(fileName);
	rapidxml::xml_document<> document;
	document.parse<0>(file.data());

	rapidxml::xml_node<>* rootNode = document.first_node(rootNodeName.data(), rootNodeName.size());

	{
		rapidxml::xml_node<>* meshesNode = rootNode->first_node(meshesNodeName.data(), meshesNodeName.size());

		for (rapidxml::xml_node<>* fileNode = meshesNode->first_node(fileNodeName.data(), fileNodeName.size());
			fileNode != nullptr;
			fileNode = fileNode->next_sibling(fileNodeName.data(), fileNodeName.size()))
		{
			result.objFileNames.emplace_back(fileNode->value());
		}
	}


	{
		rapidxml::xml_node<>* sceneNode = rootNode->first_node(sceneNodeName.data(), sceneNodeName.size());

		for (rapidxml::xml_node<>* objectNode = sceneNode->first_node(objectNodeName.data(), objectNodeName.size());
			objectNode != nullptr;
			objectNode = objectNode->next_sibling(objectNodeName.data(), objectNodeName.size()))
		{
			int meshIndex = std::atoi(objectNode->first_node(meshindexNodeName.data(), meshindexNodeName.size())->value());
			std::string name = objectNode->first_node(nameNodeName.data(), nameNodeName.size())->value();

			glm::mat4 transform;
			std::sscanf(
				objectNode->first_node(transformNodeName.data(), transformNodeName.size())->value(),
				"%f %f %f %f"
				"%f %f %f %f"
				"%f %f %f %f"
				"%f %f %f %f",

				&transform[0][0], &transform[0][1], &transform[0][2], &transform[0][3],
				&transform[1][0], &transform[1][1], &transform[1][2], &transform[1][3],
				&transform[2][0], &transform[2][1], &transform[2][2], &transform[2][3],
				&transform[3][0], &transform[3][1], &transform[3][2], &transform[3][3]
			);

			result.objects.push_back({ meshIndex, std::move(name), transform });
		}
	}

	return result;
}
