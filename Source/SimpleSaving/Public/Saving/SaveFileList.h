// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "SaveFileList.generated.h"

//=================================================================
// 
//=================================================================
USTRUCT(BlueprintType)
struct FSaveFileList
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString Filename;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime DateTime;
};