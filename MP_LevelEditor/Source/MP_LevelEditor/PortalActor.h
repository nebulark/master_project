// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalActor.generated.h"

class UStaticMeshComponent;

UCLASS(hidecategories = (Input, Movement, Physics, Collision, Lighting, Rendering, Physics, StaticMesh, Matrials, Replication, Mobile, MaterialParameters, HLOD))
class MP_LEVELEDITOR_API APortalActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	FTransform GetTransformA() const;
	FTransform GetTransformB() const;



	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Root;

	UPROPERTY(VisibleInstanceOnly)
	UStaticMeshComponent* EndpointA;

	UPROPERTY(VisibleInstanceOnly)
	UStaticMeshComponent* EndpointB;
};
