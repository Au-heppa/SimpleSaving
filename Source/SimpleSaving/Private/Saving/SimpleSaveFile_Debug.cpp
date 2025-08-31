// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Saving/SimpleSaveFile.h"

#if WITH_EDITOR

//=================================================================
// 
//=================================================================
bool DoPropertiesMatch(const FString &Old, const FString &New)
{
	if (Old.Len() > 0 && New.Len() > 0 && Old[0] == L'!' && New[0] == L'!')
	{
		FString OldLeft;
		if (!Old.Split(TEXT(":"), &OldLeft, NULL, ESearchCase::CaseSensitive, ESearchDir::FromStart))
			return false;

		FString NewLeft;
		if (!New.Split(TEXT(":"), &NewLeft, NULL, ESearchCase::CaseSensitive, ESearchDir::FromStart))
			return false;

		return OldLeft.Equals(NewLeft, ESearchCase::CaseSensitive);
	}

	if (Old.Len() > 0 && New.Len() > 0 && Old[0] == L'(' && New[0] == L'(')
		return true;

	return Old.Equals(New, ESearchCase::CaseSensitive);
}

//=================================================================
// 
//=================================================================
bool CompareSingles(const FCustomSaveData &My, const FCustomSaveData &Other)
{
	//
	for (auto It = Other.Singles.CreateConstIterator(); It; ++It)
	{
		//
		if (!My.Singles.Contains(It.Key()))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Property \"%s\" not found in new object with tag \"%s\" name \"%s\" class \"%s\"!"),
				*It.Key().ToString(),
				*Other.Tag.ToString(),
				*Other.Name.ToString(),
				*Other.Class->GetName());

			return false;
		}

		const FString &String = My.Singles[It.Key()];
		if (DoPropertiesMatch(It.Value(), String) == false)
		{
			UE_LOG(LogTemp, Fatal, TEXT("Property \"%s\" value mismatch found in new object with tag \"%s\" name \"%s\" class \"%s\"! Current value \"%s\" old value \"%s\"!"),
				*It.Key().ToString(),
				*Other.Tag.ToString(),
				*Other.Name.ToString(),
				*Other.Class->GetName(),
				*String, 
				*It.Value());
			return false;
		}
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool CompareArrays(const FCustomSaveData &My, const FCustomSaveData &Other)
{
	//
	for (auto It = Other.Arrays.CreateConstIterator(); It; ++It)
	{
		//
		if (!My.Arrays.Contains(It.Key()))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Array Property \"%s\" not found in new object with tag \"%s\" name \"%s\" class \"%s\"!"),
				*It.Key().ToString(),
				*Other.Tag.ToString(),
				*Other.Name.ToString(),
				*Other.Class->GetName());

			return false;
		}

		const FArrayData &Array = My.Arrays[It.Key()];

		if (Array.Data.Num() != It.Value().Data.Num())
		{
			UE_LOG(LogTemp, Fatal, TEXT("Array Property \"%s\" size mismatch found in new object with tag \"%s\" name \"%s\" class \"%s\"! Current size %d old size %d!"),
				*It.Key().ToString(),
				*Other.Tag.ToString(),
				*Other.Name.ToString(),
				*Other.Class->GetName(),
				Array.Data.Num(),
				It.Value().Data.Num());
			return false;
		}

		for (int32 i=0; i<Array.Data.Num(); i++)
		{
			if (DoPropertiesMatch(It.Value().Data.GetData()[i], Array.Data.GetData()[i]) == false)
			{
				UE_LOG(LogTemp, Fatal, TEXT("Array Property \"%s\" value mismatch found in new object with tag \"%s\" name \"%s\" class \"%s\"! Index %d Current value \"%s\" old value \"%s\"!"),
					*It.Key().ToString(),
					*Other.Tag.ToString(),
					*Other.Name.ToString(),
					*Other.Class->GetName(),
					i,
					*Array.Data.GetData()[i], 
					*It.Value().Data.GetData()[i]);
				return false;
			}
		}
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool CompareMaps(const FCustomSaveData &My, const FCustomSaveData &Other)
{
	//
	for (auto It = Other.Maps.CreateConstIterator(); It; ++It)
	{
		//
		if (!My.Maps.Contains(It.Key()))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Map Property \"%s\" not found in new object with tag \"%s\" name \"%s\" class \"%s\"!"),
				*It.Key().ToString(),
				*Other.Tag.ToString(),
				*Other.Name.ToString(),
				*Other.Class->GetName());

			return false;
		}

		const FMapData &Map = My.Maps[It.Key()];

		if (Map.Data.Num() != It.Value().Data.Num())
		{
			UE_LOG(LogTemp, Fatal, TEXT("Map Property \"%s\" size mismatch found in new object with tag \"%s\" name \"%s\" class \"%s\"! Current size %d old size %d!"),
				*It.Key().ToString(),
				*Other.Tag.ToString(),
				*Other.Name.ToString(),
				*Other.Class->GetName(),
				Map.Data.Num(),
				It.Value().Data.Num());
			return false;
		}

		//Go through the map
		for (auto It2 = It.Value().Data.CreateConstIterator(); It2; ++It2)
		{
			if (Map.Data.Contains(It2.Key()))
			{
				UE_LOG(LogTemp, Fatal, TEXT("Map Property \"%s\" value mismatch found in new object with tag \"%s\" name \"%s\" class \"%s\"! Value \"%s\" not found in new!"),
					*It.Key().ToString(),
					*Other.Tag.ToString(),
					*Other.Name.ToString(),
					*Other.Class->GetName(),
					*It2.Key())
				return false;
			}

			if (DoPropertiesMatch(It2.Value(), Map.Data[It2.Key()]) == false)
			{
				UE_LOG(LogTemp, Fatal, TEXT("Map Property \"%s\" value mismatch found in new object with tag \"%s\" name \"%s\" class \"%s\"! Key \"%s\" Current value \"%s\" old value \"%s\"!"),
					*It.Key().ToString(),
					*Other.Tag.ToString(),
					*Other.Name.ToString(),
					*Other.Class->GetName(),
					*It2.Key(),
					*Map.Data[It2.Key()], 
					*It2.Value());
				return false;
			}
		}
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool CompareCustomSaveData(const FCustomSaveData &My, const FCustomSaveData &Other)
{
	if (CompareSingles(My, Other) == false)
		return false;

	return true;
}

//=================================================================
// 
//=================================================================
FORCEINLINE int32 GetMyObjectIndex(const FCustomSaveData &Other, const TArray<class UObject*> &InMyObjects, const TArray<class UObject*> &InOtherObjects)
{
	if (!InOtherObjects.IsValidIndex(Other.ObjectIndex))
		return INDEX_NONE;

	class UObject *pOther = InOtherObjects.GetData()[Other.ObjectIndex];
	if (!IsValid(pOther))
		return INDEX_NONE;

	return InMyObjects.Find(pOther);
}

//=================================================================
// 
//=================================================================
bool CompareArrayCustomSaveData(const TArray<FCustomSaveData> &InMy, const TArray<FCustomSaveData> &InOther, const TArray<class UObject*> &InMyObjects, const TArray<class UObject*> &InOtherObjects)
{
	for (int32 i=0; i<InOther.Num(); i++)
	{
		const FCustomSaveData &Other = InOther.GetData()[i];
		int32 iMyIndex = GetMyObjectIndex(Other, InMyObjects, InOtherObjects);
		int32 iMyData = FindCustomDataIndex(iMyIndex, InMy);
		if (iMyData == INDEX_NONE)
		{
			UE_LOG(LogTemp, Fatal, TEXT("Failed to find object with tag \"%s\" name \"%s\" class \"%s\" in current object list!"),
					*Other.Tag.ToString(),
					*Other.Name.ToString(),
					*Other.Class->GetName());
			return false;
		}

		const FCustomSaveData &My = InMy.GetData()[iMyData];

		//Compare the actual data
		if (!CompareCustomSaveData(My, Other))
		{
			return false;
		}
	}

	return true;
}

//=================================================================
// 
//=================================================================
FORCEINLINE class UObject *GetObjectFromArray(const FCustomSaveData &InData, const TArray<class UObject*> &InObjects)
{
	if (InObjects.IsValidIndex(InData.ObjectIndex))
	{
		return InObjects.GetData()[InData.ObjectIndex];
	}

	return NULL;
}

//=================================================================
// 
//=================================================================
bool CompareActorSaveData(const TArray<FActorSaveData> &InMyActors, const TArray<FActorSaveData> &InOtherActors, const TArray<class UObject*> &InMyObjects, const TArray<class UObject*> &InOtherObjects)
{
	//Go through other actors
	for (int32 i=0; i<InOtherActors.Num(); i++)
	{
		const FActorSaveData &Other = InOtherActors.GetData()[i];
		int32 iMyIndex = GetMyObjectIndex(Other.Custom, InMyObjects, InOtherObjects);
		int32 iMyData = FindActorDataIndex(iMyIndex, InMyActors);
		if (iMyData == INDEX_NONE)
		{
			UE_LOG(LogTemp, Fatal, TEXT("Failed to find actor with tag \"%s\" name \"%s\" class \"%s\" in current object list! Old object \"%s\" [%d/%d]. My index [%d/%d]. My data list size: %d."),
					*Other.Custom.Tag.ToString(),
					*Other.Custom.Name.ToString(),
					*Other.Custom.Class->GetName(),
					InOtherObjects.IsValidIndex(Other.Custom.ObjectIndex) && IsValid(InOtherObjects.GetData()[Other.Custom.ObjectIndex]) ? *InOtherObjects.GetData()[Other.Custom.ObjectIndex]->GetName() : TEXT("NULL"),
					Other.Custom.ObjectIndex, InOtherObjects.Num(),
					iMyIndex, InMyObjects.Num(),
					InMyActors.Num());
			return false;
		}		

		const FActorSaveData &My = InMyActors.GetData()[iMyData];

		//Compare the actual data
		if (!CompareCustomSaveData(My.Custom, Other.Custom))
		{
			return false;
		}
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::DebugSaveGame(const class UObject *WorldContext, bool InMultiLevel)
{
	//
	class USaveGameInstance *pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("DebugSaveGame: No game instance!"));
		return false;
	}

	class USimpleSaveFile *pCompareTo = pGameInstance->GetLoadGame();
	if (!IsValid(pCompareTo) || pCompareTo == this)
	{
		UE_LOG(LogTemp, Error, TEXT("DebugSaveGame: No load game to compare to!"));
		return false;
	}

	SaveData(WorldContext, InMultiLevel, false);

	FName MapName = *UGameplayStatics::GetCurrentLevelName(WorldContext);

	//Compare global actors data
	if (!CompareActorSaveData(GetGlobalActors(), pCompareTo->GetGlobalActors(), GetGlobalSaveObjects(), pCompareTo->GetGlobalSaveObjects()))
	{
		return false;
	}

	//Compare global custom objects
	if (!CompareArrayCustomSaveData(GetCustomObjects(), pCompareTo->GetCustomObjects(), GetGlobalSaveObjects(), pCompareTo->GetGlobalSaveObjects()))
	{
		return false;
	}

	const FLevelSaveData *pMyLevelData = GetLevelSaveData(MapName);
	if (pMyLevelData == NULL)
	{
		UE_LOG(LogTemp, Fatal, TEXT("No current level data!"));
	}

	const FLevelSaveData *pOtherLevelData = pCompareTo->GetLevelSaveData(MapName);
	if (pOtherLevelData == NULL)
	{
		UE_LOG(LogTemp, Fatal, TEXT("No old level data!"));
	}

	//Compare level actor data
	if (!CompareActorSaveData(pMyLevelData->Actors, pOtherLevelData->Actors, GetLocalSaveObjects(), pCompareTo->GetLocalSaveObjects()))
	{
		return false;
	}

	//Compare level custom objects data
	if (!CompareArrayCustomSaveData(pMyLevelData->CustomObjects, pOtherLevelData->CustomObjects, GetLocalSaveObjects(), pCompareTo->GetLocalSaveObjects()))
	{
		return false;
	}

	return true;
}

#endif //