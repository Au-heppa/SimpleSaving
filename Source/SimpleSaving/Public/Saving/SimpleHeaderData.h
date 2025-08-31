// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "GameplayTagContainer.h"
#include "SimpleHeaderData.generated.h"

//==============================================================================================================
// 
//==============================================================================================================
USTRUCT(BlueprintType)
struct FSimpleHeaderData
{
	GENERATED_USTRUCT_BODY()

	//Autosaved
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Filename;

	//Autosaved
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString MapName;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<class UMaterialInterface> Icon;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer GameplayTags;

	//
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer LevelChangeTags;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<uint8> Data;
};