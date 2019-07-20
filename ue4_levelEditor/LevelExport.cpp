// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelExport.h"
#include "Editor/EditorEngine.h"
#include "Editor.h"
#include "EngineUtils.h"

#include "Components/StaticMeshComponent.h"
#include "Exporters/StaticMeshExporterOBJ.h"
#include "OutputDeviceFile.h"
#include "Paths.h"
#include "EditorLevelLibrary.h"
#include "Misc/FileHelper.h"
#include "CString.h"

#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

namespace
{
	struct LevelObject
	{
		int32 meshId;
		FString name;
		FTransform transform;
	};

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
}



void ULevelExport::ExportLevel()
{


	TArray<UStaticMesh*> staticMeshes;

	TArray<LevelObject> levelObjects;


	EActorIteratorFlags flags = EActorIteratorFlags::OnlyActiveLevels | EActorIteratorFlags::SkipPendingKill;

	TArray<AActor*> actors = UEditorLevelLibrary::GetAllLevelActors();

	
	for (AActor* actor : actors)
	{
		if (UStaticMeshComponent* staticMeshComponent = actor->FindComponentByClass<UStaticMeshComponent>())
		{
			UStaticMesh* staticMesh = staticMeshComponent->GetStaticMesh();
			const int32 foundIndex = staticMeshes.AddUnique(staticMesh);
			levelObjects.Add({ foundIndex, actor->GetName(), actor->GetTransform() });
		}
	}


	// create obj files
	for (UStaticMesh* mesh : staticMeshes)
	{
		FString fileName = FPaths::ProjectContentDir() + mesh->GetName() + ".obj";
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

		rapidxml::xml_node<>* scene = doc.allocate_node(rapidxml::node_element, "scene");
		rootNode->append_node(scene);


		char buffer[1024] = {};
		for (const LevelObject& lvObject : levelObjects)
		{

			rapidxml::xml_node<>* objectNode = doc.allocate_node(rapidxml::node_element, "object");
			scene->append_node(objectNode);

			{
				int meshIndexCharCount = snprintf(buffer, std::size(buffer), "%i", lvObject.meshId);
				char* meshIndexString = doc.allocate_string(buffer, meshIndexCharCount);
				objectNode->append_node(doc.allocate_node(rapidxml::node_element, "meshindex", meshIndexString, 0, meshIndexCharCount));
			}
			{
				char* nameString = doc.allocate_string(TCHAR_TO_ANSI(*lvObject.name));
				objectNode->append_node(doc.allocate_node(rapidxml::node_element, "name", nameString));
			}
			{
				
				FMatrix mat = lvObject.transform.ToMatrixWithScale();
				int matAsStringLength = std::snprintf(buffer, std::size(buffer),
					"%f %f %f %f"
					"%f %f %f %f"
					"%f %f %f %f"
					"%f %f %f %f",

					mat.M[0][0], mat.M[0][1], mat.M[0][2], mat.M[0][3],
					mat.M[1][0], mat.M[1][1], mat.M[1][2], mat.M[1][3],
					mat.M[2][0], mat.M[2][1], mat.M[2][2], mat.M[2][3],
					mat.M[3][0], mat.M[3][1], mat.M[3][2], mat.M[3][3]
					);

				check(matAsStringLength < std::size(buffer));

				char* matAsString = doc.allocate_string(buffer, matAsStringLength);
				objectNode->append_node(doc.allocate_node(rapidxml::node_element, "transform", matAsString, 0, matAsStringLength));
			}
		}

		{
			FString fileName = FPaths::ProjectContentDir() + "level.xml";
			fileName.Replace(TEXT("/"), TEXT("\\"));
	
			std::ofstream file;
			file.open(TCHAR_TO_ANSI(*fileName));
			file << doc;
		}
		
	
	}

	// create c++ script

}

