#include "pch.hpp"
#include "LevelLoader.hpp"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"

#include "Transform.hpp"

namespace
{
	namespace NodeNames
	{
		const char transform[] = "transform";
		const char meshIndex[] = "meshindex";
		const char position[] = "position";
		const char scale[] = "scale";
		const char quaternion[] = "quat";
		const char name[] = "name";
		const char object[] = "object";
		const char portal[] = "portal";
		const char camera[] = "camera";
		const char target[] = "target";

		const char root[] = "root";
		const char meshes[] = "meshes";
		const char file[] = "file";
		const char scene[] = "scene";
	}

	int ReadMeshIndex(const rapidxml::xml_node<>* meshIdNode)
	{
		return std::atoi(meshIdNode->value());
	}

	glm::vec3 Vec3FromString(const char* string)
	{
		glm::vec3 result;
		std::sscanf(string, "%f %f %f", &result.x, &result.y, &result.z);
		return result;
	}

	glm::vec3 ReadVecNode(const rapidxml::xml_node<>* vecNode)
	{
		return Vec3FromString(vecNode->value());
	}

	glm::quat QuatFromString(const char* string)
	{
		glm::quat result;
		std::sscanf(string, "%f %f %f %f", &result.x, &result.y, &result.z, &result.w);
		return result;
	}

	glm::quat ReadQuatNode(const rapidxml::xml_node<>* quatNode)
	{
		return QuatFromString(quatNode->value());
	}

	Transform ReadTransformNode(const rapidxml::xml_node<>* transformNode)
	{
		return Transform(
			ReadVecNode(transformNode->first_node(NodeNames::position)),
			ReadVecNode(transformNode->first_node(NodeNames::scale)),
			ReadQuatNode(transformNode->first_node(NodeNames::quaternion))
		);
	}

	std::string ReadStringNode(const rapidxml::xml_node<>* stringNode)
	{
		return std::string(stringNode->value());
	}

	glm::mat4 Mat4FromString(const char* string)
	{
		glm::mat4 transform;
		std::sscanf(
			string,
			"%f %f %f %f"
			"%f %f %f %f"
			"%f %f %f %f"
			"%f %f %f %f",

			&transform[0][0], &transform[0][1], &transform[0][2], &transform[0][3],
			&transform[1][0], &transform[1][1], &transform[1][2], &transform[1][3],
			&transform[2][0], &transform[2][1], &transform[2][2], &transform[2][3],
			&transform[3][0], &transform[3][1], &transform[3][2], &transform[3][3]
		);

		return transform;
	}
}

LevelLoader::LoadLevelResult LevelLoader::LoadLevel(const char* fileName)
{

	LoadLevelResult result;

	rapidxml::file<> file(fileName);
	rapidxml::xml_document<> document;
	document.parse<0>(file.data());

	rapidxml::xml_node<>* rootNode = document.first_node(NodeNames::root);

	{
		rapidxml::xml_node<>* meshesNode = rootNode->first_node(NodeNames::meshes);

		for (rapidxml::xml_node<>* fileNode = meshesNode->first_node(NodeNames::file);
			fileNode != nullptr;
			fileNode = fileNode->next_sibling(NodeNames::file))
		{
			result.objFileNames.emplace_back(fileNode->value());
		}
	}


	{
		rapidxml::xml_node<>* sceneNode = rootNode->first_node(NodeNames::scene);

		for (rapidxml::xml_node<>* objectNode = sceneNode->first_node(NodeNames::object);
			objectNode != nullptr;
			objectNode = objectNode->next_sibling(NodeNames::object))
		{
			result.objects.push_back({
				ReadMeshIndex(objectNode->first_node(NodeNames::meshIndex)),
				ReadStringNode(objectNode->first_node(NodeNames::name)),
				ReadTransformNode(objectNode->first_node(NodeNames::transform)) 
				});
		}

		for (rapidxml::xml_node<>* portalNode = sceneNode->first_node(NodeNames::portal);
			portalNode != nullptr;
			portalNode = portalNode->next_sibling(NodeNames::portal))
		{

			PortalObject portal;
			portal.meshId = ReadMeshIndex(portalNode->first_node(NodeNames::meshIndex));
			portal.name = ReadStringNode(portalNode->first_node(NodeNames::name));

			rapidxml::xml_node<>* firstTransformNode = portalNode->first_node(NodeNames::transform);
			portal.transformA = ReadTransformNode(firstTransformNode);
			portal.transformB = ReadTransformNode(firstTransformNode->next_sibling(NodeNames::transform));

			result.portals.push_back(std::move(portal));
		}

		for (rapidxml::xml_node<>* cameraNode = sceneNode->first_node(NodeNames::camera);
			cameraNode != nullptr;
			cameraNode = cameraNode->next_sibling(NodeNames::camera))
		{

			CameraObject camera;
			camera.positon = ReadVecNode(cameraNode->first_node(NodeNames::position));
			camera.target = ReadVecNode(cameraNode->first_node(NodeNames::target));
			result.cameras.push_back(camera);
		}
	}

	return result;
}
