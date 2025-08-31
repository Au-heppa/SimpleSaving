// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SimpleHeaderData.h"
#include "Engine/LatentActionManager.h"
#include "SimpleSaveHeader.generated.h"

//=================================================================
// 
//=================================================================
UCLASS()
class SIMPLESAVING_API USimpleSaveHeader : public USaveGame
{
	GENERATED_BODY()
	
	//=================================================================
	// 
	//=================================================================
public:

	//
	static bool SaveHeaderDataFor(const class UObject* WorldContext, const FString &Filename);

	//
	static bool CopyHeaderData(const class UObject* WorldContext, const FString& Source, const FString & Destination);

	//
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContext"))
	static bool GetHeaderDataFor(const class UObject* WorldContext, const FString& Filename, FSimpleHeaderData &OutData);

	//
	static class USimpleSaveHeader *GetHeaderData(const class UObject* WorldContext);

public:

	//
	void Screenshot(const FString& Filename);

	//
	void AcceptScreenshot(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData);

public:

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime", meta=(AllowPrivateAccess=true))
	TArray<FSimpleHeaderData> Data;

	//
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Runtime", meta = (AllowPrivateAccess = true))
	TArray<FString> FilesRequestingScreenshot;
};
