// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelExport.h"
#include "Editor.h"
#include "EngineUtils.h"

#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "OutputDeviceFile.h"
#include "Paths.h"
#include "EditorLevelLibrary.h"
#include "Misc/FileHelper.h"
#include "CString.h"
#include "PortalActor.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "Camera/CameraActor.h"


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
		const char direction[] = "direction";
	}

	struct LevelObject
	{
		int32 meshId;
		FString name;
		FTransform transform;
	};

	struct PortalObject
	{
		int32 meshId;
		FString name;
		FTransform transformA;
		FTransform transformB;
	};

	template<size_t bufferSize>
	int VectorToString(char(&buffer)[bufferSize], const FVector& vec)
	{
		return std::snprintf(buffer, std::size(buffer), "%f %f %f", vec.X, vec.Z, vec.Y);
	}


	template<size_t bufferSize>
	int QuatToString(char(&buffer)[bufferSize], const FQuat& quat)
	{
		return std::snprintf(buffer, std::size(buffer), "%f %f %f %f", -quat.X, -quat.Z, -quat.Y, quat.W);
	}

	void AppendMeshIdNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* parent, int meshId)
	{
		char buffer[32];
		int meshIndexCharCount = snprintf(buffer, std::size(buffer), "%i", meshId);
		char* meshIndexString = doc.allocate_string(buffer, meshIndexCharCount);
		parent->append_node(doc.allocate_node(
			rapidxml::node_element,
			NodeNames::meshIndex, meshIndexString,
			std::size(NodeNames::meshIndex) - 1 , meshIndexCharCount));
	}

	void AppendNameNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* parent, const FString& name)
	{
		char* nameString = doc.allocate_string(TCHAR_TO_ANSI(*name));
		parent->append_node(doc.allocate_node(rapidxml::node_element, NodeNames::name, nameString));
	}

	void AppendVec(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* parent, const FVector& vec,  const char* nodeName)
	{
		char buffer[256];

		int vecCharCount = VectorToString(buffer, vec);
		char* vecString = doc.allocate_string(buffer, vecCharCount);
		parent->append_node(
			doc.allocate_node(rapidxml::node_element,
				nodeName, vecString,
				0, vecCharCount));
	}

	void AppendTransformNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* parent, const FTransform& transform)
	{
		char buffer[512];
		rapidxml::xml_node<>* transformNode = doc.allocate_node(rapidxml::node_element, NodeNames::transform);
		parent->append_node(transformNode);

		{
			AppendVec(doc, transformNode, transform.GetLocation(), NodeNames::position);
			AppendVec(doc, transformNode, transform.GetScale3D(), NodeNames::scale);

		}
		{
			int quatStringCount = QuatToString(buffer, transform.GetRotation());

			char* quatAsString = doc.allocate_string(buffer, quatStringCount);
			transformNode->append_node(
				doc.allocate_node(
					rapidxml::node_element,
					NodeNames::quaternion, quatAsString,
					std::size(NodeNames::quaternion) - 1, quatStringCount));
		}
	}

	void ExportMesh(const UStaticMesh& mesh, FArchive& ar)
	{
		const FStaticMeshLODResources& RenderData = mesh.GetLODForExport(0);
		uint32 VertexCount = RenderData.GetNumVertices();

		check(VertexCount == RenderData.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices());

		ar.Logf(TEXT("# UnrealEd OBJ exporter (_Internal)\r\n"));
		ar.Logf(TEXT("\r\n"));
		for (uint32 i = 0; i < VertexCount; i++)
		{
			const FVector& OSPos = RenderData.VertexBuffers.PositionVertexBuffer.VertexPosition(i);
			//			const FVector WSPos = StaticMeshComponent->LocalToWorld.TransformPosition( OSPos );
			const FVector WSPos = OSPos;

			// Transform to Lightwave's coordinate system
			ar.Logf(TEXT("v %f %f %f\r\n"), WSPos.X, WSPos.Z, WSPos.Y);
		}

		ar.Logf(TEXT("\r\n"));
		for (uint32 i = 0; i < VertexCount; ++i)
		{
			// takes the first UV
			const FVector2D UV = RenderData.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, 0);

			// Invert the y-coordinate (Lightwave has their bitmaps upside-down from us).
			ar.Logf(TEXT("vt %f %f\r\n"), UV.X, 1.0f - UV.Y);
		}

		ar.Logf(TEXT("\r\n"));

		for (uint32 i = 0; i < VertexCount; ++i)
		{
			const FVector& OSNormal = RenderData.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(i);
			const FVector WSNormal = OSNormal;

			// Transform to Lightwave's coordinate system
			ar.Logf(TEXT("vn %f %f %f\r\n"), WSNormal.X, WSNormal.Z, WSNormal.Y);
		}

		{
			FIndexArrayView Indices = RenderData.IndexBuffer.GetArrayView();
			uint32 NumIndices = Indices.Num();

			check(NumIndices % 3 == 0);
			for (uint32 i = 0; i < NumIndices / 3; i++)
			{
				// Wavefront indices are 1 based
				uint32 a = Indices[3 * i + 0] + 1;
				uint32 b = Indices[3 * i + 1] + 1;
				uint32 c = Indices[3 * i + 2] + 1;

				ar.Logf(TEXT("f %d/%d/%d %d/%d/%d %d/%d/%d\r\n"),
					a, a, a,
					b, b, b,
					c, c, c);
			}
		}

	}

	template<size_t arraySize>
	int TransformToString(char(&buffer)[arraySize], const FTransform& transform)
	{
		const FMatrix mat = transform.ToMatrixWithScale();
		int matAsStringLength = std::snprintf(buffer, std::size(buffer),
			"%f %f %f %f"
			"%f %f %f %f"
			"%f %f %f %f"
			"%f %f %f %f",

			// flip y and z to convert from ue coordinate system to ours
			mat.M[0][0], mat.M[0][2], mat.M[0][1], mat.M[0][3],
			mat.M[2][0], mat.M[2][2], mat.M[2][1], mat.M[2][3],
			mat.M[1][0], mat.M[1][2], mat.M[1][1], mat.M[1][3],
			mat.M[3][0], mat.M[3][2], mat.M[3][1], mat.M[3][3]

			/*
				does not work, ue4 has different coordinate system
				mat.M[0][0], mat.M[0][1], mat.M[0][2], mat.M[0][3],
				mat.M[1][0], mat.M[1][1], mat.M[1][2], mat.M[1][3],
				mat.M[2][0], mat.M[2][1], mat.M[2][2], mat.M[2][3],
				mat.M[3][0], mat.M[3][1], mat.M[3][2], mat.M[3][3]*/
		);

		check(matAsStringLength < std::size(buffer));
		return matAsStringLength;
	}
}



void ULevelExport::ExportLevel()
{
	UWorld* editorWorld =	UEditorLevelLibrary::GetEditorWorld();
	FString exportFolderName = FString("level/") + editorWorld->GetMapName() + "/";
	TArray<UStaticMesh*> staticMeshes;

	TArray<LevelObject> levelObjects;
	TArray<PortalObject> portalObjects;


	EActorIteratorFlags flags = EActorIteratorFlags::OnlyActiveLevels | EActorIteratorFlags::SkipPendingKill;

	TArray<AActor*> actors = UEditorLevelLibrary::GetAllLevelActors();

	struct SceneCamera
	{
		FVector position;
		FVector dir;
	};

	TArray<SceneCamera> cameras;

	for (AActor* actor : actors)
	{

		if (APortalActor * PortalActor = Cast<APortalActor>(actor))
		{
			UStaticMesh* staticMesh = PortalActor->Root->GetStaticMesh();
			const int32 foundIndex = staticMeshes.AddUnique(staticMesh);
			portalObjects.Add({ foundIndex, PortalActor->GetName(), PortalActor->GetTransformA(), PortalActor->GetTransformB() });
		}
		else if (AStaticMeshActor * staticMeshActor = Cast<AStaticMeshActor>(actor))
		{
			UStaticMesh* staticMesh = staticMeshActor->GetStaticMeshComponent()->GetStaticMesh();
			const int32 foundIndex = staticMeshes.AddUnique(staticMesh);
			levelObjects.Add({ foundIndex, actor->GetName(), actor->GetTransform() });
		}
		else if (ACameraActor * cameraActor = Cast<ACameraActor>(actor))
		{
			const FVector cameraLocation = cameraActor->GetActorLocation();
			const FVector cameraDir = cameraActor->GetActorForwardVector();

			cameras.Add({
				cameraLocation,
				cameraDir
				});
		}
	}


	// create obj files
	for (UStaticMesh* mesh : staticMeshes)
	{
		FString fileName = FPaths::ProjectContentDir() + exportFolderName + mesh->GetName() + ".obj";
		TUniquePtr<FArchive> file(IFileManager::Get().CreateFileWriter(*fileName));
		ExportMesh(*mesh, *file);
	}
	{
		rapidxml::xml_document<> doc;
		rapidxml::xml_node<>* rootNode = doc.allocate_node(rapidxml::node_element, "root");
		doc.append_node(rootNode);
		rootNode->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));

		rapidxml::xml_node<>* meshesNode = doc.allocate_node(rapidxml::node_element, "meshes");
		rootNode->append_node(meshesNode);
		for (int i = 0; i < staticMeshes.Num(); ++i)
		{
			FString fileName = staticMeshes[i]->GetName() + ".obj";
			meshesNode->append_node(doc.allocate_node(rapidxml::node_element, "file", doc.allocate_string(TCHAR_TO_ANSI(*fileName))));
		}

		rapidxml::xml_node<>* sceneNode = doc.allocate_node(rapidxml::node_element, "scene");
		rootNode->append_node(sceneNode);


		for (const LevelObject& lvObject : levelObjects)
		{

			rapidxml::xml_node<>* objectNode = doc.allocate_node(rapidxml::node_element, NodeNames::object);
			sceneNode->append_node(objectNode);

			::AppendMeshIdNode(doc, objectNode, lvObject.meshId);
			::AppendTransformNode(doc, objectNode, lvObject.transform);
			::AppendNameNode(doc, objectNode, lvObject.name);
		}

		for (const PortalObject& portal : portalObjects)
		{
			rapidxml::xml_node<>* portalNode = doc.allocate_node(rapidxml::node_element, NodeNames::portal);
			sceneNode->append_node(portalNode);

			AppendMeshIdNode(doc, portalNode, portal.meshId);
			AppendNameNode(doc, portalNode, portal.name);
			AppendTransformNode(doc, portalNode, portal.transformA);
			AppendTransformNode(doc, portalNode, portal.transformB);
		}

		for (const SceneCamera& cam : cameras)
		{
			rapidxml::xml_node<>* cameraNode = doc.allocate_node(rapidxml::node_element, NodeNames::camera);
			sceneNode->append_node(cameraNode);
			AppendVec(doc, cameraNode, cam.position, NodeNames::position);
			AppendVec(doc, cameraNode, cam.dir, NodeNames::direction);
		}


		{
			FString fileName = FPaths::ProjectContentDir() + exportFolderName + "level.xml";
			fileName.Replace(TEXT("/"), TEXT("\\"));

			std::ofstream file;
			file.open(TCHAR_TO_ANSI(*fileName));
			file << doc;
		}


	}

	// create c++ script

}

