// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "SaveData.generated.h"

//=================================================================
// 
//=================================================================
USTRUCT(BlueprintType)
struct FArrayData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FString> Data;
};

//=================================================================
// 
//=================================================================
USTRUCT(BlueprintType)
struct FMapData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FString, FString> Data;
};

//==============================================================================================================
// No saving object properties
//==============================================================================================================
USTRUCT(BlueprintType)
struct FSimpleSaveData
{
	GENERATED_USTRUCT_BODY()

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FString> Singles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FArrayData> Arrays;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FMapData> Maps;

	//
	FORCEINLINE int32 GetCount() const { return Singles.Num() + Arrays.Num() + Maps.Num(); }
};

//==============================================================================================================
// No saving object properties
//==============================================================================================================
USTRUCT(BlueprintType)
struct FSimpleActorData
{
	GENERATED_USTRUCT_BODY()

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FString> Singles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FArrayData> Arrays;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FMapData> Maps;

	//
	FORCEINLINE int32 GetCount() const { return Singles.Num() + Arrays.Num() + Maps.Num(); }
};

//==============================================================================================================
//
//==============================================================================================================
USTRUCT(BlueprintType)
struct FCustomSaveData
{
	GENERATED_USTRUCT_BODY()

public:

	static int32 _CalculateBytes(const TMap<FName, FString>& InSingles, const TMap<FName, FArrayData>& InArrays, const TMap<FName, FMapData>& InMaps);
	static void _GetSize(const TMap<FName, FString> &InSingles, const TMap<FName, FArrayData> &InArrays, const TMap<FName, FMapData> &InMaps, int32 &OutMegabytes, int32 &OutKilobytes, int32 &OutBytes);
	static void _ParseBytes(int32 InBytes, int32 &OutMegabytes, int32 &OutKilobytes, int32 &OutBytes);

	//Get size of the data
	FORCEINLINE void GetSize(int32 &OutMegabytes, int32 &OutKilobytes, int32 &OutBytes) const
	{
		_GetSize(Singles, Arrays, Maps, OutMegabytes, OutKilobytes, OutBytes);
	}

	//
	static void _GetSizeString(int32 InMegabytes, int32 InKilobytes, int32 InBytes, FString &OutString);

	//Get size string of the data
	FORCEINLINE FString GetSizeString() const 
	{
		FString OutString;
		int32 iMegas, iKilos, iBytes;
		GetSize(iMegas, iKilos, iBytes);
		_GetSizeString(iMegas, iKilos, iBytes, OutString);
		return OutString;
	}

public:

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FString> Singles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FArrayData> Arrays;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FMapData> Maps;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Name = NAME_None;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Tag = NAME_None;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSoftClassPtr<class UObject> Class = NULL;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ObjectIndex = INDEX_NONE;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 OuterObjectIndex = INDEX_NONE;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool OuterIsGlobal = false;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool Recreate = false;

	//
	FORCEINLINE int32 GetCount() const { return Singles.Num() + Arrays.Num() + Maps.Num(); }
};

//==============================================================================================================
//
//==============================================================================================================
USTRUCT(BlueprintType)
struct FComponentSaveData
{
	GENERATED_USTRUCT_BODY()

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTransform Transform;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FCustomSaveData Custom;
};

//==============================================================================================================
//
//==============================================================================================================
USTRUCT(BlueprintType)
struct FActorSaveData
{
	GENERATED_USTRUCT_BODY()

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTransform Transform;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTransform RelativeTransform;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AttachSocketName;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FComponentSaveData> Components;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FCustomSaveData Custom;
};

//=================================================================
// 
//=================================================================
USTRUCT(BlueprintType)
struct FLevelSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName LevelName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FActorSaveData> Actors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FCustomSaveData> CustomObjects;

	UPROPERTY(VisibleAnywhere)
	float SaveTime = 0.0f;
};