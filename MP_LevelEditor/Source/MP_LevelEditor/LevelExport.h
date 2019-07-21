// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LevelExport.generated.h"

/**
 * 
 */
UCLASS()
class MP_LEVELEDITOR_API ULevelExport : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "Export Level")
	static void ExportLevel();
	
};
