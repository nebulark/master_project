// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

#include "ConstructorHelpers.h"

// Sets default values
APortalActor::APortalActor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> defaultMesh(TEXT("/Engine/EditorMeshes/EditorPlane"));
	

	Root = CreateDefaultSubobject<UStaticMeshComponent>(FName("Root"));
	RootComponent = Root;
	Root->SetStaticMesh(defaultMesh.Object);
	Root->SetVisibility(false);
	

	EndpointA = CreateDefaultSubobject<UStaticMeshComponent>(FName("Portal Endpoint A"));
	EndpointA->SetupAttachment(Root);
	EndpointA->SetStaticMesh(Root->GetStaticMesh());

	
	EndpointB = CreateDefaultSubobject<UStaticMeshComponent>(FName("Portal Endpoint B"));
	EndpointB->SetupAttachment(Root);
	EndpointB->SetStaticMesh(Root->GetStaticMesh());
}

void APortalActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EndpointA->SetStaticMesh(Root->GetStaticMesh());
	EndpointB->SetStaticMesh(Root->GetStaticMesh());
}

FTransform APortalActor::GetTransformA() const
{
	return EndpointA->GetComponentTransform();
}

FTransform APortalActor::GetTransformB() const
{
	return EndpointB->GetComponentTransform();
}


