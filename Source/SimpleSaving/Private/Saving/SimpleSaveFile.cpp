// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Saving/SimpleSaveFile.h"
#include "Kismet/GameplayStatics.h"
#include "Saving/SaveGameInstance.h"
#include "Saving/SaveInterface.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/DataTable.h"
#include "Saving/SavedTime.h"

#if WITH_EDITOR
#include "Layers/LayersSubsystem.h"
#endif //
#include "Internationalization/Regex.h"
#include "UMG/Public/Components/Widget.h"
#include "Animation/AnimInstance.h"
#include "EngineUtils.h"
#include "Saving/SimpleSaveHeader.h"
#include "SimpleSavingLoadingScreen.h"
#include "Saving/LevelChangeInterface.h"
#include "Components/MeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/DrawFrustumComponent.h"
#include "Saving/SimpleRestoreHandler.h"
#include "Components/TimelineComponent.h"

#if WITH_EDITOR
static bool g_bIsUsingDataPointer = false;
#endif //

//=================================================================
// 
//=================================================================
int32 _GetObjectsDataSize(const FCustomSaveData &InData)
{
	int32 iTotal = 0;

	iTotal += FCustomSaveData::_CalculateBytes(InData.Singles, InData.Arrays, InData.Maps);

	iTotal += InData.Class.ToString().GetAllocatedSize() + sizeof(int32);
	iTotal += InData.Name.ToString().GetAllocatedSize() + sizeof(int32);
	iTotal += InData.Tag.ToString().GetAllocatedSize() + sizeof(int32);
	iTotal += sizeof(int32) * 2 + sizeof(bool) * 2;

	return iTotal;
}

//=================================================================
// 
//=================================================================
int32 _GetTotalObjectsDataSize(const TArray<FCustomSaveData>& InObjectsData)
{
	int32 iTotal = sizeof(int32);
	for (int32 i=0; i<InObjectsData.Num(); i++)
	{
		iTotal += _GetObjectsDataSize(InObjectsData.GetData()[i]);
	}

	return iTotal;
}

//=================================================================
// 
//=================================================================
int32 _GetActorDataSize(const FActorSaveData& InActorData)
{
	int32 iTotal = 0; 
	iTotal += InActorData.AttachSocketName.ToString().GetAllocatedSize();
	iTotal += _GetObjectsDataSize(InActorData.Custom);
	iTotal += sizeof(FTransform) * 2;


	iTotal += sizeof(int32);
	for (int32 i=0; i<InActorData.Components.Num(); i++)
	{
		const FComponentSaveData &InComponentData = InActorData.Components.GetData()[i];

		iTotal += sizeof(FTransform);
		iTotal += _GetObjectsDataSize(InComponentData.Custom);
	}

	return iTotal;
}

//=================================================================
// 
//=================================================================
int32 _GetTotalActorDataSize(const TArray<FActorSaveData>& InActorsData)
{
	int32 iTotal = sizeof(int32);
	for (int32 i=0; i<InActorsData.Num(); i++)
	{
		iTotal += _GetActorDataSize(InActorsData.GetData()[i]);
	}

	return iTotal;
}

//=================================================================
// 
//=================================================================
FString USimpleSaveFile::GetLevelSizeString(int32 InLevel) const
{
	if (!Levels.IsValidIndex(InLevel))
		return TEXT("Invalid");

	const FLevelSaveData &Data = Levels.GetData()[InLevel];

	int32 iTotal = _GetTotalActorDataSize(Data.Actors) + _GetTotalObjectsDataSize(Data.CustomObjects) + Data.LevelName.ToString().GetAllocatedSize() + sizeof(float);

	int32 Bytes;
	int32 Kilybytes;
	int32 Megabytes;
	FCustomSaveData::_ParseBytes(iTotal, Megabytes, Kilybytes, Bytes);

	FString String;
	FCustomSaveData::_GetSizeString(Megabytes, Kilybytes, Bytes, String);
	return String;
}

//=================================================================
// 
//=================================================================
FString USimpleSaveFile::GetGlobalSizeString() const
{
	int32 iTotal = _GetTotalActorDataSize(GlobalActors) + _GetTotalObjectsDataSize(CustomObjects);

	int32 Bytes;
	int32 Kilybytes;
	int32 Megabytes;
	FCustomSaveData::_ParseBytes(iTotal, Megabytes, Kilybytes, Bytes);

	FString String;
	FCustomSaveData::_GetSizeString(Megabytes, Kilybytes, Bytes, String);
	return String;
}

//=================================================================
// 
//=================================================================
FString USimpleSaveFile::GetTotalSizeString() const
{
	int32 iTotal = _GetTotalActorDataSize(GlobalActors) + _GetTotalObjectsDataSize(CustomObjects);

	for (int32 i=0; i<Levels.Num(); i++)
	{
		const FLevelSaveData& Data = Levels.GetData()[i];
		iTotal += _GetTotalActorDataSize(Data.Actors) + _GetTotalObjectsDataSize(Data.CustomObjects) + Data.LevelName.ToString().GetAllocatedSize() + sizeof(float);
	}

	int32 Bytes;
	int32 Kilybytes;
	int32 Megabytes;
	FCustomSaveData::_ParseBytes(iTotal, Megabytes, Kilybytes, Bytes);

	FString String;
	FCustomSaveData::_GetSizeString(Megabytes, Kilybytes, Bytes, String);
	return String;
}

//=================================================================
// 
//=================================================================
const FName USimpleSaveFile::Name_DontSave = TEXT("DontSave");
const FName USimpleSaveFile::Name_ForceSave = TEXT("ForceSave");

//=================================================================
// 
//=================================================================
FORCEINLINE static void AppendActorsThatReference(FString &Debug, const TArray<FActorSaveData>&InArray, class UObject* InObject, const TArray<class UObject*> &InSavedObjects)
{
	
}

//=================================================================
// 
//=================================================================
FString USimpleSaveFile::GatherObjectCrashData(class UObject* InObject)
{
	class USaveGameInstance *pInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(InObject));
	if (!pInstance)
		return TEXT("No instance");

	if (pInstance->GetLoadGame() == NULL)
		return TEXT("Not loading from save file");

	FString Debug;
	
	if (pInstance->GetLoadGame()->GetGlobalSaveObjects().Contains(InObject))
	{
		bool bGlobal = false;
		FCustomSaveData *pData = pInstance->GetLoadGame()->FindObjectToSave(NULL, InObject, bGlobal);

		Debug = FString::Printf(TEXT("Global %d/%d have data %s"),
			pInstance->GetLoadGame()->GetGlobalSaveObjects().Find(InObject),
			pInstance->GetLoadGame()->GetGlobalSaveObjects().Num(),
			pData ? TEXT("YES") : TEXT("NO"));

		//AppendActorsThatReference(Debug, pInstance->GetLoadGame()->GetGlobalActors(), InObject, pInstance->GetLoadGame()->GetGlobalSaveObjects());
	}
	else if (pInstance->GetLoadGame()->GetLocalSaveObjects().Contains(InObject))
	{
		bool bGlobal = false;

		FLevelSaveData *pLevel = pInstance->GetLoadGame()->CurrentLevelData;

		FCustomSaveData* pData = pInstance->GetLoadGame()->FindObjectToSave(pLevel, InObject, bGlobal);

		class UObject *pOuter = NULL;
		if (pData)
		{
			const TArray<class UObject*> &Array = pData->OuterIsGlobal ? pInstance->GetLoadGame()->GetGlobalSaveObjects() : pInstance->GetLoadGame()->GetLocalSaveObjects();
			if (Array.IsValidIndex(pData->OuterObjectIndex))
			{
				pOuter = Array.GetData()[pData->OuterObjectIndex];
			}
		}

		Debug = FString::Printf(TEXT("Local %d/%d have data %s outer %s"),
			pInstance->GetLoadGame()->GetLocalSaveObjects().Find(InObject),
			pInstance->GetLoadGame()->GetLocalSaveObjects().Num(),
			pData ? TEXT("YES") : TEXT("NO"),
			pOuter != NULL ? *pOuter->GetName() : TEXT("NULL"));
	}
	else
	{
		Debug = TEXT("Neither");
	}
	
	return Debug;
}

//=================================================================
// Add class to load
//=================================================================
FORCEINLINE static void AddClassFromCustomData(const FCustomSaveData& InData, TArray<class TSoftClassPtr<class UObject>>& OutClasses)
{
	if (!InData.Class.IsNull())
	{
		OutClasses.AddUnique(InData.Class);
	}
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::GatherClassesToLoad(class UObject *WorldContext, TArray<class TSoftClassPtr<class UObject>> &OutClasses)
{
	if (GatheredClasses.Num() > 0)
	{
		OutClasses = GatheredClasses;
		return true;
	}

	OutClasses.Reset();

	//Global actors
	for (int32 i=0; i<GlobalActors.Num(); i++)
	{
		AddClassFromCustomData(GlobalActors.GetData()[i].Custom, OutClasses);

		//
		for (int32 j=0; j<GlobalActors.GetData()[i].Components.Num(); j++)
		{
			AddClassFromCustomData(GlobalActors.GetData()[i].Components.GetData()[j].Custom, OutClasses);
		}
	}

	//Global data
	for (int32 i=0; i<CustomObjects.Num(); i++)
	{
		AddClassFromCustomData(CustomObjects.GetData()[i], OutClasses);
	}


	FName MapName = *UGameplayStatics::GetCurrentLevelName(WorldContext);

	//Find level data
	FLevelSaveData *pLevelData = NULL;
	for (int32 i=0; i<Levels.Num(); i++)
	{
		if (Levels.GetData()[i].LevelName == MapName)
		{
			pLevelData = &Levels.GetData()[i];
			break;
		}
	}

	if (pLevelData != NULL)
	{
		for (int32 i=0; i<pLevelData->Actors.Num(); i++)
		{
			AddClassFromCustomData(pLevelData->Actors.GetData()[i].Custom, OutClasses);

			//
			for (int32 j=0; j<pLevelData->Actors.GetData()[i].Components.Num(); j++)
			{
				AddClassFromCustomData(pLevelData->Actors.GetData()[i].Components.GetData()[j].Custom, OutClasses);
			}
		}

		for (int32 i=0; i<pLevelData->CustomObjects.Num(); i++)
		{
			AddClassFromCustomData(pLevelData->CustomObjects.GetData()[i], OutClasses);
		}
	}

	return OutClasses.Num() > 0;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::AutoSave(const class UObject *WorldContext, bool InMultiLevel)
{
	if (!IsValid(WorldContext))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveFile::SaveGame: No world context!"));
		return false;
	}

	if (!CanSave(WorldContext))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveFile::CanSave returns false!"));
		return false;
	}

	//static const FString String_Autosave = TEXT("Autosave");
	//static const FString String_AutosaveBackUp = TEXT("AutosaveBackup");
	//CopySaveFile(WorldContext, String_Autosave, String_AutosaveBackUp);

	FString MapName = WorldContext->GetWorld()->GetOutermost()->GetName();
	MapName = WorldContext->GetWorld()->RemovePIEPrefix(MapName);
	StripLevelNameString(MapName, MapName);

	return SaveGame(WorldContext, FString::Printf(TEXT("Autosave_%s"), *MapName), InMultiLevel);
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::CopySaveFile(const class UObject *WorldContext, FString Source, FString Destination)
{
	if (!IsValid(WorldContext))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveFile::SaveGame: No world context!"));
		return false;
	}

	//
	class USaveGameInstance *pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveFile::SaveGame: No game instance!"));
		return false;
	}

	//
	class USimpleSaveFile *pLoadGame = Cast<USimpleSaveFile>(UGameplayStatics::LoadGameFromSlot(Source, 0));
	if (!pLoadGame)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load game \"%s\""), *Source);
		return false;
	}

	//
	if (UGameplayStatics::SaveGameToSlot(pLoadGame, Destination, 0))
	{
		USimpleSaveHeader::CopyHeaderData(WorldContext, Source, Destination);

		pGameInstance->UpdateSaveFile(Destination);
		return true;
	}

	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::SaveGame(const class UObject *WorldContext, FString Filename, bool InMultiLevel, bool InChangeLevel)
{
	if (!IsValid(WorldContext))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::SaveGame: No world context!"));
		return false;
	}

	if (!CanSave(WorldContext))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveFile::CanSave returns false!"));
		return false;
	}

	//
	class USaveGameInstance *pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::SaveGame: No game instance!"));
		return false;
	}

	//If we already have a save game
	class USimpleSaveFile *pSaveGame = NULL;
	if (!IsValid(pGameInstance->GetLoadGame()))
	{
		//
		pGameInstance->SetSaveGame(Cast<USimpleSaveFile>(UGameplayStatics::CreateSaveGameObject(USimpleSaveFile::StaticClass())));
		if (!IsValid(pGameInstance->GetLoadGame()))
		{
			UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::SaveGame: Failed to create save game \"%s\""), *Filename);
			return false;
		}
	}

	float InSaveTime = pGameInstance->GetTotalTime(WorldContext);

	//Save the data
	if (!pGameInstance->GetLoadGame()->SaveData(WorldContext, InMultiLevel, InSaveTime, true))
	{
		return false;
	}

	//
	if (!InChangeLevel && UGameplayStatics::SaveGameToSlot(pGameInstance->GetLoadGame(), Filename, 0))
	{
		USimpleSaveHeader::SaveHeaderDataFor(WorldContext, Filename);

		pGameInstance->UpdateSaveFile(Filename);
		return true;
	}

	if (InChangeLevel)
		return true;

	return false;
}

//==================================================================================================
// 
//==================================================================================================
float USaveGameInstance::GetTotalTime(const class UObject * const WorldContextObject) const
{
	return TotalTime + UGameplayStatics::GetTimeSeconds(WorldContextObject);
}

//==================================================================================================
// 
//==================================================================================================
float USaveGameInstance::GetTotalTimeSeconds(const UObject* WorldContextObject)
{
	class USaveGameInstance *pInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
	return pInstance != NULL ? pInstance->GetTotalTime(WorldContextObject) : 0.0f;
}

//==================================================================================================
// 
//==================================================================================================
FString USimpleSaveFile::ParseSaveFilename(FString InFilename)
{	
	if (InFilename.Len() < 1)
	{
		return "";
	}	
	
	InFilename = InFilename.Replace(TEXT("_"), TEXT(" "));

	int32 numOfCharacters = InFilename.Len();
	for (int32 i = 0; i < numOfCharacters; i++)
	{		
		FString currentCharacter = InFilename.Mid(i, 1);
		const FRegexPattern myPattern(TEXT("[^A-Za-z0-9 ]"));
		FRegexMatcher myMatcher(myPattern, InFilename);
		if (myMatcher.FindNext())
		{			
			int32 b = myMatcher.GetMatchBeginning();
			int32 e = myMatcher.GetMatchEnding();							
			InFilename.RemoveAt(b, 1, true);
		}		
	}
	InFilename = InFilename.Replace(TEXT(" "), TEXT("_"));

	return InFilename;
}

//=================================================================
// 
//=================================================================
FORCEINLINE static bool IsMarkedForDestruction(class AActor *InActor)
{
	if (!IsValid(InActor))
		return true;

	if (InActor->HasAllFlags(EObjectFlags::RF_BeginDestroyed))
		return true;

	return InActor->GetLifeSpan() > 0.0f;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::CanSave(const class UObject *WorldContextObject)
{
	if (!IsValid(WorldContextObject))
		return false;

	TArray<class AActor*> AllActors;
	UGameplayStatics::GetAllActorsWithInterface(WorldContextObject, USaveInterface::StaticClass(), AllActors);
	for (int32 i=0; i<AllActors.Num(); i++)
	{
		if (IsMarkedForDestruction(AllActors.GetData()[i]))
			continue;

		class ISaveInterface *pInterface = Cast<ISaveInterface>(AllActors.GetData()[i]);
		if (pInterface && pInterface->BlockSaving())
			return false;
	}

	return true;
}

//=================================================================
// 
//=================================================================
const FName USimpleSaveFile::Name_GameInstance = TEXT("GameInstance");
const FName USimpleSaveFile::Name_PlayerController = TEXT("PlayerController");
const FName USimpleSaveFile::Name_PlayerPawn = TEXT("PlayerPawn");
const FName USimpleSaveFile::Name_GameState = TEXT("GameState");

//=================================================================
// 
//=================================================================
FCustomSaveData *USimpleSaveFile::FindObjectToSave(FLevelSaveData *InLevelData, class UObject *InObject, bool &OutGlobal)
{
	if (InLevelData != NULL)
	{
		int32 i = LocalSaveObjects.Find(InObject);
		if (LocalSaveObjects.IsValidIndex(i))
		{
			OutGlobal = false;
			FCustomSaveData *pData = FindCustomData(i, InLevelData->CustomObjects);
			if (pData != nullptr)
				return pData;

			FActorSaveData *pActor = FindActorData(i, InLevelData->Actors);
			if (pActor != nullptr)
				return &pActor->Custom;

			UE_LOG(LogTemp, Fatal, TEXT("No data for object \"%s\""), *InObject->GetName());
		}
	}

	int32 i = GlobalSaveObjects.Find(InObject);
	if (GlobalSaveObjects.IsValidIndex(i))
	{
		OutGlobal = true;
		FCustomSaveData *pData = FindCustomData(i, CustomObjects);
		if (pData != nullptr)
			return pData;

		FActorSaveData *pActor = FindActorData(i, GlobalActors);
		if (pActor != nullptr)
			return &pActor->Custom;

		UE_LOG(LogTemp, Fatal, TEXT("No data for object \"%s\""), *InObject->GetName());
	}

	return NULL;
}

//=================================================================
// 
//=================================================================
FCustomSaveData *USimpleSaveFile::AddObjectToSave(FLevelSaveData *InLevelData, class UObject *InObject, const FName &InTag)
{
	if (InObject->GetClass() == UBlueprintGeneratedClass::StaticClass())
	{
		UE_LOG(LogTemp, Fatal, TEXT("Can't save blueprint generated class!"));
	}

	int32 i = LocalSaveObjects.Find(InObject);
	if (i != INDEX_NONE)
	{
		return FindCustomData(i, InLevelData->CustomObjects);
	}

	i = GlobalSaveObjects.Find(InObject);
	if (i != INDEX_NONE)
	{
		return FindCustomData(i, CustomObjects);
	}

	FCustomSaveData NewData;

	//Figure out tag
	if (!InTag.IsNone())
	{
		NewData.Tag = InTag;
	}
	else
	{
		class ISaveInterface *pInterface = Cast<ISaveInterface>(InObject);
		if (pInterface)
		{
			NewData.Tag = pInterface->GetSavingTag();
		}
		else
		{
			NewData.Tag = NAME_None;
		}
	}

	if (InLevelData)
	{
#if WITH_EDITOR
		if (GlobalSaveObjects.Contains(InObject))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Object \"%s\" already in global objects array!"), *InObject->GetName());
		}

		if (LocalSaveObjects.Contains(InObject))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Object \"%s\" already in local objects array!"), *InObject->GetName());
		}
#endif

		NewData.ObjectIndex = LocalSaveObjects.Add(InObject);
		i = InLevelData->CustomObjects.Add(NewData);
		return &InLevelData->CustomObjects.GetData()[i];
	}

#if WITH_EDITOR
	if (GlobalSaveObjects.Contains(InObject))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Object \"%s\" already in global objects array!"), *InObject->GetName());
	}

	if (LocalSaveObjects.Contains(InObject))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Object \"%s\" already in local objects array!"), *InObject->GetName());
	}
#endif

	NewData.ObjectIndex = GlobalSaveObjects.Add(InObject);
	i = CustomObjects.Add(NewData);
	return &CustomObjects.GetData()[i];
}

//=================================================================
// 
//=================================================================
FCustomSaveData *FindAnyData(int32 ObjectIndex, TArray<FActorSaveData> &InArray)
{
	//
	for (int32 j=0; j<InArray.Num(); j++)
	{
		//If it's the actor then just return that
		if (InArray.GetData()[j].Custom.ObjectIndex == ObjectIndex)
			return &InArray.GetData()[j].Custom;

		//Go through all the components
		for (int32 k=0; k<InArray.GetData()[j].Components.Num(); k++)
		{
			if (InArray.GetData()[j].Components.GetData()[k].Custom.ObjectIndex == ObjectIndex)
			{
				return &InArray.GetData()[j].Components.GetData()[k].Custom;
			}
		}
	}

	return NULL;
}

//=================================================================
// 
//=================================================================
FORCEINLINE static bool DoesMatchTag(const FName &InTag, int32 InIndex, const FCustomSaveData &InData)
{
	if (!InTag.IsNone())
		return InData.Tag == InTag;

	return InData.ObjectIndex == InIndex;
}

//=================================================================
// 
//=================================================================
class UObject *USimpleSaveFile::GetObjectByTag(const FName &InTag, int32 InIndex, bool InGlobal) const
{
	int32 Index = INDEX_NONE;

	const TArray<FCustomSaveData> &DynamicObjects = InGlobal ? CustomObjects : CurrentLevelData->CustomObjects;
	for (int32 i=0; i<DynamicObjects.Num(); i++)
	{
		if (DoesMatchTag(InTag, InIndex, DynamicObjects.GetData()[i]))
		{
			Index = DynamicObjects.GetData()[i].ObjectIndex;
			break;
		}
	}

	//
	if (Index == INDEX_NONE)
	{
		const TArray<FActorSaveData> &Actors = InGlobal ? GlobalActors : CurrentLevelData->Actors;
		for (int32 i=0; i<Actors.Num(); i++)
		{
			if (DoesMatchTag(InTag, InIndex, Actors.GetData()[i].Custom))
			{
				Index = Actors.GetData()[i].Custom.ObjectIndex;
				break;
			}

			//Go through components
			for (int32 j=0; j<Actors.GetData()[i].Components.Num(); j++)
			{
				if (DoesMatchTag(InTag, InIndex, Actors.GetData()[i].Components.GetData()[j].Custom))
				{
					Index = Actors.GetData()[i].Components.GetData()[j].Custom.ObjectIndex;
					break;
				}
			}

			if (Index != INDEX_NONE)
				break;
		}
	}

	if (Index == INDEX_NONE)
		return NULL;

	//Return 
	if (InGlobal)
	{
		return GlobalSaveObjects.GetData()[Index];
	}

	return LocalSaveObjects.GetData()[Index];
}

//=================================================================
// 
//=================================================================
FORCEINLINE static void RestoreObjectInArray(class UObject *InObject, int32 InIndex, TArray<class UObject*> &InArray)
{
	if (InIndex >= InArray.Num())
	{
		InArray.SetNum(InIndex+1);
	}
			
	InArray.GetData()[InIndex] = InObject;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::RestoreObjectByTag(class UObject *InObject, const FName &InTag, bool InGlobal)
{
	if (InTag.IsNone())
		return false;

	if (!InGlobal && !CurrentLevelData)
		return false;

	const TArray<FCustomSaveData> &InData = InGlobal ? CustomObjects : CurrentLevelData->CustomObjects;
	for (int32 i=0; i<InData.Num(); i++)
	{
		if (InData.GetData()[i].Tag == InTag)
		{
			if (InGlobal)
			{
				RestoreObjectInArray(InObject, InData.GetData()[i].ObjectIndex, GlobalSaveObjects);
			}
			else
			{
				RestoreObjectInArray(InObject, InData.GetData()[i].ObjectIndex, LocalSaveObjects);
			}
			return true;
		}
	}

	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::RestoreActorByTag(class AActor *InActor, const FName &InTag, bool InGlobal)
{
	if (InTag.IsNone())
		return false;

	if (!InGlobal && !CurrentLevelData)
		return false;

	const TArray<FActorSaveData> &InData = InGlobal ? GlobalActors : CurrentLevelData->Actors;
	for (int32 i=0; i<InData.Num(); i++)
	{
		if (InData.GetData()[i].Custom.Tag == InTag)
		{
			if (InGlobal)
			{
				RestoreObjectInArray(InActor, InData.GetData()[i].Custom.ObjectIndex, GlobalSaveObjects);
			}
			else
			{
				RestoreObjectInArray(InActor, InData.GetData()[i].Custom.ObjectIndex, LocalSaveObjects);
			}

			//
			TArray<class UActorComponent*> InComponents;
			InActor->GetComponents(InComponents);


			//Go through components
			for (int32 j=0; j<InComponents.Num(); j++)
			{
				FName ComponentTag = *FString::Printf(TEXT("%s.%s"), *InTag.ToString(), *InComponents.GetData()[j]->GetName());

				int32 iComponentIndex = INDEX_NONE;
				for (int32 k=0; k<InData.GetData()[i].Components.Num(); k++)
				{
					if (InData.GetData()[i].Components.GetData()[k].Custom.Tag == ComponentTag)
					{
						iComponentIndex = InData.GetData()[i].Components.GetData()[k].Custom.ObjectIndex;
						break;
					}
				}

				if (iComponentIndex == INDEX_NONE)
				{
					//UE_LOG(LogTemp, Error, TEXT("Failed to restore component with tag \"%s\""), *ComponentTag.ToString());
					continue;
				}

				if (InGlobal)
				{
					RestoreObjectInArray(InComponents.GetData()[j], iComponentIndex, GlobalSaveObjects);
				}
				else
				{
					RestoreObjectInArray(InComponents.GetData()[j], iComponentIndex, LocalSaveObjects);
				}
			}
			
			return true;
		}
	}

	UE_LOG(LogTemp, Fatal, TEXT("Failed to restore actor by tag \"%s\""), *InTag.ToString());
	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::GetObjectTag(class UObject *InObject, FName &OutTag, int32 &OutIndex, bool &OutGlobal)
{
	FCustomSaveData *pData = NULL;

#if WITH_EDITOR
	bool bDebug = false; //InObject->GetName().Contains(TEXT("Master"));
	if (bDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("Object \"%s\" with local index %d/%d global index %d/%d in %s"),
			*InObject->GetName(),
			LocalSaveObjects.Find(InObject), LocalSaveObjects.Num(),
			GlobalSaveObjects.Find(InObject), GlobalSaveObjects.Num(), *GetName());
	}
#endif

	int32 i = GlobalSaveObjects.Find(InObject);
	if (i != INDEX_NONE)
	{
		pData = FindAnyData(i, GlobalActors);
		if (pData)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Found %s in global actor data!"), *InObject->GetName());
		}
		else
		{
			pData = FindCustomData(i, CustomObjects);
		}	

		OutGlobal = true;
	}
	else if (!CurrentLevelData)
	{
		UE_LOG(LogTemp, Fatal, TEXT("No current level data!"));
	}
	else //if (CurrentLevelData)
	{
		i = LocalSaveObjects.Find(InObject);
		if (i != INDEX_NONE)
		{
			pData = FindAnyData(i, CurrentLevelData->Actors);
			if (pData)
			{
				//UE_LOG(LogTemp, Warning, TEXT("Found %s in local actor data!"), *InObject->GetName());
			}
			else
			{
				pData = FindCustomData(i, CurrentLevelData->CustomObjects);
			}
		}

		OutGlobal = false;
	}

#if WITH_EDITOR
	if (i != INDEX_NONE && !pData)
	{
		UE_LOG(LogTemp, Fatal, TEXT("Index %d for object \"%s\" but not found in array %s!"), i, *InObject->GetName(), OutGlobal ? TEXT("Global") : TEXT("Local"));
	}
#endif //
	
	if (pData)
	{
		OutTag = pData->Tag;
		OutIndex = pData->ObjectIndex;
		return true;
	}

	return false;
}

//=================================================================
// 
//=================================================================
FActorSaveData *USimpleSaveFile::AddActorToSave(FLevelSaveData *InLevelData, class AActor *InActor, const FName &InTag)
{
	int32 i = LocalSaveObjects.Find(InActor);
	if (i != INDEX_NONE)
	{
		return FindActorData(i, InLevelData->Actors);
	}

	i = GlobalSaveObjects.Find(InActor);
	if (i != INDEX_NONE)
	{
		return FindActorData(i, GlobalActors);
	}

	FActorSaveData NewData;

	//Figure out tag
	if (!InTag.IsNone())
	{
		NewData.Custom.Tag = InTag;
	}
	else
	{
		class ISaveInterface *pInterface = Cast<ISaveInterface>(InActor);
		if (pInterface)
		{
			NewData.Custom.Tag = pInterface->GetSavingTag();
		}
		else
		{
			NewData.Custom.Tag = NAME_None;
		}
	}

	FActorSaveData *pData = NULL;

	if (InLevelData)
	{
#if WITH_EDITOR
		if (GlobalSaveObjects.Contains(InActor))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Object \"%s\" already in global objects array!"), *InActor->GetName());
		}

		if (LocalSaveObjects.Contains(InActor))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Object \"%s\" already in local objects array!"), *InActor->GetName());
		}

		if (g_bIsUsingDataPointer)
		{
			UE_LOG(LogTemp, Fatal, TEXT("Trying to add actor save file using a pointer!"));
		}
#endif

		NewData.Custom.ObjectIndex = LocalSaveObjects.Add(InActor);
		i = InLevelData->Actors.Add(NewData);
		pData = &InLevelData->Actors.GetData()[i];
	}
	else
	{
#if WITH_EDITOR
		if (GlobalSaveObjects.Contains(InActor))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Object \"%s\" already in global objects array!"), *InActor->GetName());
		}

		if (LocalSaveObjects.Contains(InActor))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Object \"%s\" already in local objects array!"), *InActor->GetName());
		}

		if (g_bIsUsingDataPointer)
		{
			UE_LOG(LogTemp, Fatal, TEXT("Trying to add actor save file using a pointer!"));
		}
#endif

		NewData.Custom.ObjectIndex = GlobalSaveObjects.Add(InActor);
		i = GlobalActors.Add(NewData);
		pData = &GlobalActors.GetData()[i];
	}

	TArray<class UActorComponent*> InComponents;
	InActor->GetComponents(InComponents);

	for (int32 j=0; j<InComponents.Num(); j++)
	{
		if (InComponents.GetData()[j]->ComponentTags.Contains(Name_DontSave))
			continue;

		//Never save these unless specified
		if (!InComponents.GetData()[j]->ComponentTags.Contains(Name_ForceSave))
		{
			TSubclassOf<class UActorComponent> ComponentClass = InComponents.GetData()[j]->GetClass();
			if (ComponentClass->IsChildOf(UMeshComponent::StaticClass()) || 
				ComponentClass->IsChildOf(UFXSystemComponent::StaticClass()) || 
				ComponentClass->IsChildOf(UAudioComponent::StaticClass()) ||
				ComponentClass->IsChildOf(UShapeComponent::StaticClass()) ||
				ComponentClass->IsChildOf(UArrowComponent::StaticClass()) ||
				ComponentClass->IsChildOf(UDrawFrustumComponent::StaticClass()) ||
				ComponentClass->IsChildOf(UTimelineComponent::StaticClass()))
			{
				continue;
			}
		}

		AddComponentToSave(InLevelData, InTag, pData, InComponents.GetData()[j]);
	}


	class ISaveInterface *pActorInterface = Cast<ISaveInterface>(InActor);

	//Save attached actors as well
	if (!InLevelData)
	{
		TArray<class AActor*> Attached;
		InActor->GetAttachedActors(Attached);

#if WITH_EDITOR
		UE_LOG(LogTemp, Warning, TEXT("Actor %s with tag %s has %d children"), *InActor->GetActorLabel(), *InTag.ToString(), Attached.Num());
#endif

		//Save attached actors
		for (int32 j=0; j<Attached.Num(); j++)
		{
			ISaveInterface *pInterface = Cast<ISaveInterface>(Attached.GetData()[j]);
			if (pInterface && pInterface->ShouldSave() && !IsMarkedForDestruction(Attached.GetData()[j]))
			{
				FName TagToUse = NAME_None;

				if (!InTag.IsNone() && pActorInterface)
				{
					FName AttachedTag = pActorInterface->GetAttachedTagName(Attached.GetData()[j]);
					if (!AttachedTag.IsNone())
					{
						TagToUse = *FString::Printf(TEXT("%s.%s"), *InTag.ToString(), *AttachedTag.ToString());
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("No attached tag for %s"), *InTag.ToString());
					}
				}

				AddActorToSave(InLevelData, Attached.GetData()[j], TagToUse);
			}
		}
	}

	return pData;
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::AddComponentToSave(FLevelSaveData* InLevelData, const FName& InTag, FActorSaveData* InActorData, class UActorComponent * InComponent)
{
	FComponentSaveData ComponentData;

	ComponentData.Custom.Name = InComponent->GetFName();

	if (InTag.IsNone())
	{
		ComponentData.Custom.Tag = NAME_None;
	}
	else
	{
		ComponentData.Custom.Tag = *FString::Printf(TEXT("%s.%s"), *InTag.ToString(), *InComponent->GetName());
	}

	if (InLevelData)
	{
		ComponentData.Custom.ObjectIndex = LocalSaveObjects.Add(InComponent);
	}
	else
	{
		ComponentData.Custom.ObjectIndex = GlobalSaveObjects.Add(InComponent);
	}

	//Add to actor data
	InActorData->Components.Add(ComponentData);
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::GatherAttachParents(TArray<class USceneComponent*> &AttachParents)
{
	for (int32 i = 0; i < GlobalSaveObjects.Num(); i++)
	{
		class AActor* pActor = Cast<AActor>(GlobalSaveObjects.GetData()[i]);
		if (IsValid(pActor) && pActor->GetRootComponent() && pActor->GetRootComponent()->GetAttachParent())
		{
			AttachParents.AddUnique(pActor->GetRootComponent()->GetAttachParent());
		}
	}

	for (int32 i = 0; i < LocalSaveObjects.Num(); i++)
	{
		class AActor* pActor = Cast<AActor>(LocalSaveObjects.GetData()[i]);
		if (IsValid(pActor) && pActor->GetRootComponent() && pActor->GetRootComponent()->GetAttachParent())
		{
			AttachParents.AddUnique(pActor->GetRootComponent()->GetAttachParent());
		}
	}
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::SaveData(const class UObject *WorldContext, bool InMultiLevel, float InTime, bool InCleapUp)
{
	//
	class USaveGameInstance *pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("SaveData: No game instance!"));
		return false;
	}

	//
	class APlayerController *pController = UGameplayStatics::GetPlayerController(WorldContext, 0);
	if (!IsValid(pController))
	{
		UE_LOG(LogTemp, Error, TEXT("SaveData: Failed to get player controller!"));
		return false;
	}

	//
	class APawn *pPlayer = UGameplayStatics::GetPlayerPawn(WorldContext, 0);
	if (!IsValid(pPlayer))
	{
		UE_LOG(LogTemp, Error, TEXT("SaveData: Failed to get player!"));
		return false;
	}

	class AGameStateBase *pGameState = UGameplayStatics::GetGameState(WorldContext);
	if (!IsValid(pGameState))
	{
		UE_LOG(LogTemp, Error, TEXT("SaveData: No game state!"));
		return false;
	}

	LocalSaveObjects.Reset();
	GlobalSaveObjects.Reset();
	GlobalActors.Reset();
	CustomObjects.Reset();
	CurrentLevelData = NULL;
	SaveTime = InTime;
	KeepInMemory.Reset();

	//=========================================================================================
	// GATHER ACTORS
	//=========================================================================================

	AddObjectToSave(NULL, pGameInstance, Name_GameInstance);
	AddActorToSave(NULL, pController, Name_PlayerController);
	AddActorToSave(NULL, pPlayer, Name_PlayerPawn);
	AddActorToSave(NULL, pGameState, Name_GameState);

	TMap<FName, class AActor*> CustomTags;
	pGameInstance->GetGlobalActorTags(pController, pPlayer, false, CustomTags);
	for (auto It = CustomTags.CreateConstIterator(); It; ++It)
	{
		AddActorToSave(NULL, It.Value(), It.Key());
	}

	if (!InMultiLevel)
	{
		Levels.Reset();
	}

	CurrentLevelData = SaveLevelData(pGameInstance, pController, pPlayer, WorldContext);
	CurrentLevelData->SaveTime = InTime;

	//=========================================================================================
	// SAVE ALL THE DATA
	//=========================================================================================

#if WITH_EDITOR
	g_bIsUsingDataPointer = true;
#endif //

	TArray<class USceneComponent*> AttachParents;
	GatherAttachParents(AttachParents);
	for (int32 i=0; i<AttachParents.Num(); i++)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Checking forcing component \"%s\" to save!"), *AttachParents.GetData()[i]->GetName());

		if (GlobalSaveObjects.Contains(AttachParents.GetData()[i]) || LocalSaveObjects.Contains(AttachParents.GetData()[i]))
			continue;

		class AActor *pActor = AttachParents.GetData()[i]->GetOwner();
		int32 iGlobalIndex = GlobalSaveObjects.Find(pActor);
		int32 iLocalIndex = LocalSaveObjects.Find(pActor);

		if (!GlobalSaveObjects.IsValidIndex(iGlobalIndex) && !LocalSaveObjects.IsValidIndex(iLocalIndex))
			continue;

		bool bGlobal = GlobalSaveObjects.IsValidIndex(iGlobalIndex);

		FActorSaveData *pActorData = FindActorData(bGlobal ? iGlobalIndex : iLocalIndex, bGlobal ? GlobalActors : CurrentLevelData->Actors);
		if (!pActorData)	
		{
			continue;
		}

		//UE_LOG(LogTemp, Error, TEXT("Forcing component \"%s\" on actor \"%s\" to save with data %s!"), *AttachParents.GetData()[i]->GetName(), *pActor->GetActorLabel(), *pActorData->Custom.Name.ToString());
		AddComponentToSave(bGlobal ? NULL : CurrentLevelData, pActorData->Custom.Tag, pActorData, AttachParents.GetData()[i]);

		/*
		//Add all the components in the lineage
		class USceneComponent *pComponentToSave = AttachParents.GetData()[i];
		while (pComponentToSave && pComponentToSave != pActor->GetRootComponent() && pComponentToSave->GetOwner() == pActor)
		{
			if (!GlobalSaveObjects.Contains(pComponentToSave) && !LocalSaveObjects.Contains(pComponentToSave))
			{
				UE_LOG(LogTemp, Error, TEXT("Forcing component \"%s\" on actor \"%s\" to save!"), *pComponentToSave->GetName(), *pActor->GetActorLabel());

				AddComponentToSave(CurrentLevelData, pActorData->Custom.Tag, pActorData, pComponentToSave);
			}


			pComponentToSave = pComponentToSave->GetAttachParent();
		}
		*/
	}

	//Save global actors
	for (int32 i=0; i<GlobalSaveObjects.Num(); i++)
	{
		FActorSaveData *pActorData = FindActorData(i, GlobalActors);
		if (pActorData)
		{
			class AActor *pActor = Cast<AActor>(GlobalSaveObjects.GetData()[i]);
			SaveActor(pActor, *pActorData);
		}
	}

	//Save local actors
	for (int32 i=0; i<LocalSaveObjects.Num(); i++)
	{
		FActorSaveData *pActorData = FindActorData(i, CurrentLevelData->Actors);
		if (pActorData)
		{
			SaveActor(Cast<AActor>(LocalSaveObjects.GetData()[i]), *pActorData);
		}
	}

	//Go through all the dynamic objects
	int32 iNextGlobal = 0;
	while (iNextGlobal < GlobalSaveObjects.Num())
	{
		FCustomSaveData *pData = FindCustomData(iNextGlobal, CustomObjects);
		if (pData)
		{
			SaveCustomData(GlobalSaveObjects.GetData()[iNextGlobal], *pData);
		}

		iNextGlobal++;
	}

	//Go through all the dynamic objects
	int32 iNextLocal = 0;
	while (iNextLocal < LocalSaveObjects.Num())
	{
		FCustomSaveData *pData = FindCustomData(iNextLocal, CurrentLevelData->CustomObjects);
		if (pData)
		{
			SaveCustomData(LocalSaveObjects.GetData()[iNextLocal], *pData);
		}

		iNextLocal++;
	}

#if WITH_EDITOR
	g_bIsUsingDataPointer = false;
#endif //

	//=========================================================================================
	// DEBUG
	//=========================================================================================

#if WITH_EDITOR
	/*
	//
	for (int32 i=0; i<GlobalActors.Num(); i++)
	{
		UE_LOG(LogTemp, Display, TEXT("Saved global actor \"%s\" of class \"%s\" with tag \"%s\" and size: %s"), 
		*GlobalActors.GetData()[i].Custom.Name.ToString(), 
		*GlobalActors.GetData()[i].Custom.Class->GetName(), 
		*GlobalActors.GetData()[i].Custom.Tag.ToString(), 
		*GlobalActors.GetData()[i].Custom.GetSizeString()); 
	}

	//
	for (int32 i=0; i<CustomObjects.Num(); i++)
	{
		UE_LOG(LogTemp, Display, TEXT("Saved global object \"%s\" class \"%s\" with tag \"%s\" and size: %s"), 
		*CustomObjects.GetData()[i].Name.ToString(), 
		*CustomObjects.GetData()[i].Class->GetName(), 
		*CustomObjects.GetData()[i].Tag.ToString(), 
		*CustomObjects.GetData()[i].GetSizeString());
	}

	//
	for (int32 i=0; i<CurrentLevelData->Actors.Num(); i++)
	{
		UE_LOG(LogTemp, Display, TEXT("Saved local actor \"%s\" class \"%s\" with tag \"%s\" and size: %s"), 
		*CurrentLevelData->Actors.GetData()[i].Custom.Name.ToString(), 
		*CurrentLevelData->Actors.GetData()[i].Custom.Class->GetName(), 
		*CurrentLevelData->Actors.GetData()[i].Custom.Tag.ToString(),
		*CurrentLevelData->Actors.GetData()[i].Custom.GetSizeString()); 
	}

	//
	for (int32 i=0; i<CurrentLevelData->CustomObjects.Num(); i++)
	{
		UE_LOG(LogTemp, Display, TEXT("Saved local object \"%s\" class \"%s\" with tag \"%s\" and size: %s"), 
		*CurrentLevelData->CustomObjects.GetData()[i].Name.ToString(), 
		*CurrentLevelData->CustomObjects.GetData()[i].Class->GetName(), 
		*CurrentLevelData->CustomObjects.GetData()[i].Tag.ToString(),
		*CurrentLevelData->CustomObjects.GetData()[i].GetSizeString());
	}
	*/
#endif

	//=========================================================================================
	// SAVE OUTERS AND PARENTS
	//=========================================================================================

	//Save outers on global custom objects
	for (int32 i=0; i<CustomObjects.Num(); i++)
	{
		SaveObjectOuters(CustomObjects.GetData()[i], true);
	}

	//Save outers on local custom objects
	for (int32 i=0; i<CurrentLevelData->CustomObjects.Num(); i++)
	{
		SaveObjectOuters(CurrentLevelData->CustomObjects.GetData()[i], false);
	}

	//Save attach parents of global actors
	for (int32 i=0; i<GlobalActors.Num(); i++)
	{
		SaveActorAttachParents(GlobalActors.GetData()[i], true);
	}

	//Save attach parents of local actors
	for (int32 i=0; i<CurrentLevelData->Actors.Num(); i++)
	{
		SaveActorAttachParents(CurrentLevelData->Actors.GetData()[i], false);
	}

	//=========================================================================================
	// CLEAN UP & FINISH
	//=========================================================================================

	//Make sure these don't get saved
	if (InCleapUp)
	{
		GlobalSaveObjects.Reset();
		LocalSaveObjects.Reset();
		KeepInMemory.Reset();
		CurrentLevelData = NULL;
	}

	//
	//pGameInstance->SetSaveGame(this);

	UE_LOG(LogTemp, Display, TEXT("Saved %d levels %d global actors and %d custom global objects!"), 
		Levels.Num(), 
		GlobalActors.Num(),
		CustomObjects.Num());

		/*
	for (int32 i=0; i<AssetsToLoad.Num(); i++)
	{
		UE_LOG(LogTemp, Error, TEXT("Assets to load: \"%s\""), *AssetsToLoad.GetData()[i].ToString());
	}
	*/

	return true;
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::SaveObjectOuters(FCustomSaveData &InData, bool InGlobal)
{
	const TArray<class UObject*> &SaveObjects = InGlobal ? GlobalSaveObjects : LocalSaveObjects;
	class UObject *pObject = SaveObjects.GetData()[InData.ObjectIndex];

	class UObject *pOuter = pObject->GetOuter();

	InData.OuterObjectIndex = GlobalSaveObjects.Find(pOuter);
	if (InData.OuterObjectIndex != INDEX_NONE)
	{
		//UE_LOG(LogTemp, Error, TEXT("%s Saving global outer object %d with object %s from list of %d GlobalSaveObjects"), *pObject->GetName(),  InData.OuterObjectIndex, *pOuter->GetName(), GlobalSaveObjects.Num());

		InData.OuterIsGlobal = true;
		return;
	}

	InData.OuterObjectIndex = LocalSaveObjects.Find(pOuter);
	InData.OuterIsGlobal = false;

	//UE_LOG(LogTemp, Error, TEXT("%s Saving local outer object %d with object %s from list of %d GlobalSaveObjects"), *pObject->GetName(), InData.OuterObjectIndex, *pOuter->GetName(), LocalSaveObjects.Num());
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::SaveActorAttachParents(FActorSaveData &InData, bool InGlobal)
{
	const TArray<class UObject*> &SaveObjects = InGlobal ? GlobalSaveObjects : LocalSaveObjects;

#if WITH_EDITOR
	if (!SaveObjects.IsValidIndex(InData.Custom.ObjectIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("Actor \"%s\" has invalid index %d"), *InData.Custom.Name.ToString(), InData.Custom.ObjectIndex);
	}
#endif //

	class AActor *pActor = Cast<AActor>(SaveObjects.GetData()[InData.Custom.ObjectIndex]);
	if (!IsValid(pActor))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Object was not actor at %d!"), InData.Custom.ObjectIndex);
		return;	
	}

	class USceneComponent *pParent = pActor->GetRootComponent() != NULL ? pActor->GetRootComponent()->GetAttachParent() : NULL;
	if (pParent)
	{
		InData.Custom.OuterObjectIndex = GlobalSaveObjects.Find(pParent);
		if (InData.Custom.OuterObjectIndex != INDEX_NONE)
		{
			InData.Custom.OuterIsGlobal = true;
		}
		else
		{
			InData.Custom.OuterObjectIndex = LocalSaveObjects.Find(pParent);
			InData.Custom.OuterIsGlobal = false;
		}

		InData.RelativeTransform = pActor->GetRootComponent()->GetRelativeTransform();
		InData.AttachSocketName = pActor->GetAttachParentSocketName();

		/*
		UE_LOG(LogTemp, Error, TEXT("%s saving \"%s\" (%d) as attach parent"), 
		*pActor->GetName(), 
		*pParent->GetOwner()->GetName(),
		InData.Custom.OuterObjectIndex);
		*/
	}
	else
	{
		InData.AttachSocketName = NAME_None;
		InData.Custom.OuterObjectIndex = INDEX_NONE;
	}
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::GetRequiredPointers(	class UObject* WorldContext, 
											class USaveGameInstance* &OutGameInstance,
											class APlayerController *&OutController,
											class APawn *&OutPlayer,
											class AGameStateBase *&OutGameState )
{
	//
	OutGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(OutGameInstance))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::HandleRestore: No game instance!"));
		return false;
	}

	//
	OutController = UGameplayStatics::GetPlayerController(WorldContext, 0);
	if (!IsValid(OutController))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::HandleRestore: Failed to get player controller!"));
		return false;
	}

	//
	OutPlayer = UGameplayStatics::GetPlayerPawn(WorldContext, 0);
	if (!IsValid(OutPlayer))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::HandleRestore: Failed to get player!"));
		return false;
	}

	OutGameState = UGameplayStatics::GetGameState(WorldContext);
	if (!IsValid(OutGameState))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::HandleRestore: No game state!"));
		return false;
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestore_BasicObjects(class UObject* WorldContext, float& OutTimeSkip, TMap<FName, class AActor*>& CustomTags)
{
	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();

	LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Starting restore...")));

	//
	class USaveGameInstance* pGameInstance = NULL;
	class APlayerController* pController = NULL;
	class APawn* pPlayer = NULL;
	class AGameStateBase* pGameState = NULL;
	if (!GetRequiredPointers(WorldContext, pGameInstance, pController, pPlayer, pGameState))
	{
		return false;
	}

	GlobalSaveObjects.Reset();
	LocalSaveObjects.Reset();

	SetCurrentMapName(WorldContext);

	bool bLevelChange = pGameInstance->InLevelChange();

	//Find level data
	CurrentLevelData = NULL;
	for (int32 i = 0; i < Levels.Num(); i++)
	{
		if (Levels.GetData()[i].LevelName == CurrentMapName)
		{
			CurrentLevelData = &Levels.GetData()[i];
			break;
		}
	}

	//If somehow we didn't have the data
	if (!CurrentLevelData && !bLevelChange)
	{
		UE_LOG(LogTemp, Fatal, TEXT("Level data not found!"));
		return false;
	}

	pGameInstance->TotalTime = SaveTime;

	if (bLevelChange && CurrentLevelData)
	{
		OutTimeSkip += SaveTime - CurrentLevelData->SaveTime;
	}
	else
	{
		OutTimeSkip = 0.0f;
	}

	LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Restoring basic objects...")));

	RestoreObjectByTag(pGameInstance, Name_GameInstance, true);
	RestoreActorByTag(pController, Name_PlayerController, true);
	RestoreActorByTag(pPlayer, Name_PlayerPawn, true);
	RestoreActorByTag(pGameState, Name_GameState, true);

	pGameInstance->GetGlobalActorTags(pController, pPlayer, true, CustomTags);
	for (auto It = CustomTags.CreateConstIterator(); It; ++It)
	{
		RestoreActorByTag(It.Value(), It.Key(), true);
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestore_ClearObjects(class UObject* WorldContext)
{
	//
	class USaveGameInstance *pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::HandleRestore: No game instance!"));
		return false;
	}

	//Destroy actors that we need to recreate (items, props that might get destroyed)
	if (CurrentLevelData)
	{
		bool bLevelChange = pGameInstance->InLevelChange();

		ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
		LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Destroying old actors...")));

		ClearActorsOnRestore(WorldContext, bLevelChange);
	}
	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestore_RecreateAllObjects(class UObject *WorldContext, TMap<FName, class AActor*>& CustomTags)
{
	//
	class USaveGameInstance* pGameInstance = NULL;
	class APlayerController* pController = NULL;
	class APawn* pPlayer = NULL;
	class AGameStateBase* pGameState = NULL;
	if (!GetRequiredPointers(WorldContext, pGameInstance, pController, pPlayer, pGameState))
	{
		return false;
	}

	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
	LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Respawning actors...")));

	//Respawn global actors that need recreation (items, props, anything dropped by NPCs or otherwise)
	RespawnOrFindActors(WorldContext, GlobalActors, GlobalSaveObjects, true);


	//Respawn local actors that need recreation (items, props, anything dropped by NPCs or otherwise)
	if (CurrentLevelData)
	{
		pGameInstance->GetLocalActorTags(pController, pPlayer, CustomTags);
		for (auto It = CustomTags.CreateConstIterator(); It; ++It)
		{
			RestoreActorByTag(It.Value(), It.Key(), false);
		}

		RespawnOrFindActors(WorldContext, CurrentLevelData->Actors, LocalSaveObjects, false);
	}

	LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Recreating dynamic objects...")));

	//Recreate global dynamic objects
	RecreateDynamicObjects(CurrentLevelData, CustomObjects, true);

	//Recreate local dynamic objects
	if (CurrentLevelData)
	{
		RecreateDynamicObjects(CurrentLevelData, CurrentLevelData->CustomObjects, false);
	}

	return true;
}

//=================================================================
// 
//=================================================================
FORCEINLINE static int32 CalculateLoadPercentage(int32 Index, int32 Total)
{
	return ((float)(Index + 1) / (float)Total) * 100.0f;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestore_RestoreActors(class UObject* WorldContext, int32 &Start, int32 MaxCount)
{
	/*
	if (Start == -2)
		return false;
		*/

	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
	LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Restoring actor data...")));

	int32 iTotalCount = GlobalActors.Num();
	if (CurrentLevelData)
	{
		iTotalCount += CurrentLevelData->Actors.Num();
	}
	if (MaxCount <= 0)
	{
		MaxCount = iTotalCount;
	}

	//Restore local actors
	int32 iGlobalsStart = Start;
	int32 iGlobalEnd = FMath::Min(iGlobalsStart + MaxCount, GlobalActors.Num());
	for (int32 i = iGlobalsStart; i < iGlobalEnd; i++)
	{
		int32 iActorIndex = i;
		LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(FString::Printf(TEXT("Restoring actor %d%%"), CalculateLoadPercentage(iActorIndex, iTotalCount))));

		double StartTime = FPlatformTime::Seconds();

		int32 iIndex = GlobalActors.GetData()[i].Custom.ObjectIndex;
		RestoreActor(CurrentLevelData, Cast<AActor>(GlobalSaveObjects.GetData()[iIndex]), GlobalActors.GetData()[i]);

		double EndTime = FPlatformTime::Seconds();

		if (EndTime - StartTime > 0.5)
		{
			UE_LOG(LogTemp, Error, TEXT("SimpleSaveFile: Actor %s took %f seconds to restore with size %s!"), *GlobalSaveObjects.GetData()[iIndex]->GetName(), (float)(EndTime - StartTime), *GlobalActors.GetData()[i].Custom.GetSizeString());
		}
	}

	//Restore local actors
	if (CurrentLevelData)
	{
		int32 iLocalStart = FMath::Max(Start - GlobalActors.Num(), 0);
		int32 iLocalEnd = FMath::Min(iLocalStart + MaxCount, CurrentLevelData->Actors.Num());
		for (int32 i = iLocalStart; i < iLocalEnd; i++)
		{
			double StartTime = FPlatformTime::Seconds();

			int32 iActorIndex = i + GlobalActors.Num();
			LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(FString::Printf(TEXT("Restoring actor %d%%"), CalculateLoadPercentage(iActorIndex, iTotalCount))));

			int32 iIndex = CurrentLevelData->Actors.GetData()[i].Custom.ObjectIndex;
			RestoreActor(CurrentLevelData, Cast<AActor>(LocalSaveObjects.GetData()[iIndex]), CurrentLevelData->Actors.GetData()[i]);

			double EndTime = FPlatformTime::Seconds();

			if (EndTime - StartTime > 0.5)
			{
				UE_LOG(LogTemp, Error, TEXT("SimpleSaveFile: Actor %s took %f seconds to restore with size %s!"), *LocalSaveObjects.GetData()[iIndex]->GetName(), (float)(EndTime - StartTime), *CurrentLevelData->Actors.GetData()[i].Custom.GetSizeString());
			}
		}
	}

	Start += MaxCount;
	return Start >= iTotalCount;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestore_RestoreDynamicObjects(class UObject* WorldContext, int32& Start, int32 MaxCount)
{
	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
	LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Restoring custom objects...")));

	int32 iTotalCount = CustomObjects.Num();
	if (CurrentLevelData)
	{
		iTotalCount += CurrentLevelData->CustomObjects.Num();
	}
	if (MaxCount <= 0)
	{
		MaxCount = iTotalCount;
	}

	//Restore global dynamic objects	
	int32 iGlobalsStart = Start;
	int32 iGlobalEnd = FMath::Min(iGlobalsStart + MaxCount, CustomObjects.Num());
	for (int32 i = iGlobalsStart; i < iGlobalEnd; i++)
	{
		int32 iActorIndex = i;
		LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(FString::Printf(TEXT("Restoring custom object %d%%"), CalculateLoadPercentage(iActorIndex, iTotalCount))));

		int32 iIndex = CustomObjects.GetData()[i].ObjectIndex;
		RestoreCustomData(GlobalSaveObjects.GetData()[iIndex], CustomObjects.GetData()[i]);
	}

	//Restore local dynamic objects
	if (CurrentLevelData)
	{
		int32 iLocalStart = FMath::Max(Start - CustomObjects.Num(), 0);
		int32 iLocalEnd = FMath::Min(iLocalStart + MaxCount, CurrentLevelData->CustomObjects.Num());
		for (int32 i = iLocalStart; i < iLocalEnd; i++)
		{
			int32 iActorIndex = i + CustomObjects.Num();
			LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(FString::Printf(TEXT("Restoring custom object %d%%"), CalculateLoadPercentage(iActorIndex, iTotalCount))));

			int32 iIndex = CurrentLevelData->CustomObjects.GetData()[i].ObjectIndex;
			RestoreCustomData(LocalSaveObjects.GetData()[iIndex], CurrentLevelData->CustomObjects.GetData()[i]);
		}
	}


	Start += MaxCount;
	return Start >= iTotalCount;
}

//=================================================================
// Calculate which actor is where we will spawn as we enter to the new level
//=================================================================
bool USimpleSaveFile::HandleRestore_CalculateLevelChangeActor(class UObject* WorldContext)
{
	//
	class USaveGameInstance* pGameInstance = NULL;
	class APlayerController* pController = NULL;
	class APawn* pPlayer = NULL;
	class AGameStateBase* pGameState = NULL;
	if (!GetRequiredPointers(WorldContext, pGameInstance, pController, pPlayer, pGameState))
	{
		return false;
	}

	class AActor* pLevelChangeActor = NULL;

	//
	if (pGameInstance->InLevelChange())
	{
		ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
		LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Handling level change...")));

		TArray<class AActor*> AllActors;
		UGameplayStatics::GetAllActorsWithInterface(WorldContext, ULevelChangeInterface::StaticClass(), AllActors);
		for (int32 i = 0; i < AllActors.Num(); i++)
		{
			if (ILevelChangeInterface::Execute_OnLevelChange(AllActors.GetData()[i], pController, pPlayer, pGameInstance->GetLevelChangePositionTag()))
			{
				pLevelChangeActor = AllActors.GetData()[i];
				break;
			}
		}
	}

	pGameInstance->LevelChangeActor = pLevelChangeActor;
	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestore_CallOnRestore(class UObject* WorldContext)
{
	//
	class USaveGameInstance* pGameInstance = NULL;
	class APlayerController* pController = NULL;
	class APawn* pPlayer = NULL;
	class AGameStateBase* pGameState = NULL;
	if (!GetRequiredPointers(WorldContext, pGameInstance, pController, pPlayer, pGameState))
	{
		return false;
	}

	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
	LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Calling restore on objects...")));

	//Globally saved objects
	for (int32 i = 0; i < GlobalSaveObjects.Num(); i++)
	{
		class ISaveInterface* pInterface = Cast<ISaveInterface>(GlobalSaveObjects.GetData()[i]);
		if (pInterface)
		{
			pInterface->OnRestore(pGameInstance, pController);
		}
	}

	//Locally saved objects
	for (int32 i = 0; i < LocalSaveObjects.Num(); i++)
	{
		class ISaveInterface* pInterface = Cast<ISaveInterface>(LocalSaveObjects.GetData()[i]);
		if (pInterface)
		{
			pInterface->OnRestore(pGameInstance, pController);
		}
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestore_Finish(class UObject* WorldContext, bool InTriggerPostLevelChange)
{
	CurrentLevelData = NULL;

	GatheredClasses.Reset();

	if (InTriggerPostLevelChange)
	{
		USimpleSaveFile::TriggerPostLevelChange(WorldContext);
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestore(class UObject *WorldContext, float &OutTimeSkip, bool InTriggerPostLevelChange)
{
	//
	class USaveGameInstance* pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	class APlayerController* pController = UGameplayStatics::GetPlayerController(WorldContext, 0);
	class APawn* pPlayer = UGameplayStatics::GetPlayerPawn(WorldContext, 0);
	class AGameStateBase* pGameState = UGameplayStatics::GetGameState(WorldContext);
	if (!GetRequiredPointers(WorldContext, pGameInstance, pController, pPlayer, pGameState))
	{
		return false;
	}
	
	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();

	bool bLevelChange = pGameInstance->InLevelChange();

	TMap<FName, class AActor*> CustomTags;

	//Restoring basic objects
	if (!HandleRestore_BasicObjects(WorldContext, OutTimeSkip, CustomTags))
	{
		return false;
	}

	//
	if (!HandleRestore_ClearObjects(WorldContext))
	{
		return false;
	}

	if (!HandleRestore_RecreateAllObjects(WorldContext, CustomTags))
	{
		return false;
	}

	int32 Start = 0;
	if (!HandleRestore_RestoreActors(WorldContext, Start, 0))
	{
		return false;
	}

	Start = 0;
	if (!HandleRestore_RestoreDynamicObjects(WorldContext, Start, 0))
	{
		return false;
	}

	//Calculate which actor is where we will spawn as we enter to the new level
	if (!HandleRestore_CalculateLevelChangeActor(WorldContext))
	{
		return false;
	}

	//
	if (!HandleRestore_CallOnRestore(WorldContext))
	{
		return false;
	}

	//
	if (!HandleRestore_Finish(WorldContext, InTriggerPostLevelChange))
	{
		return false;
	}

	return true;
}

//=================================================================
// Do a lil bit every frame, not everything in one go
//=================================================================
void ASimpleRestoreHandler::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (Progress)
	{

	case 0:
	{
		//
		File->HandleRestore_ClearObjects(this);
		Progress++;
		break;
	}

	case 1:
	{
		File->HandleRestore_RecreateAllObjects(this, CustomTags);
		Progress++;
		break;
	}

	case 2:
	{
		if (File->HandleRestore_RestoreActors(this, ActorsRestoreStart, 3))
		{
			Progress++;
		}
		break;
	}

	case 3:
	{
		if (File->HandleRestore_RestoreDynamicObjects(this, ObjectsRestoreStart, 3))
		{
			Progress++;
		}
		break;
	}

	case 4:
	{
		File->HandleRestore_CalculateLevelChangeActor(this);
		Progress++;
		break;
	}

	case 5:
	{
		File->HandleRestore_CallOnRestore(this);
		Progress++;
		break;
	}

	case 6:
	{
		File->HandleRestore_Finish(this, TriggerPostLevelChange);
		Progress++;
		break;
	}

	//Call finish and clean up
	default:
	{
		GameInstance->FinishLoading(this);

		File = NULL;
		GameInstance = NULL;
		Destroy();
		break;
	}
	}
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::TriggerPostLevelChange(class UObject *WorldContext)
{
	//
	class USaveGameInstance *pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::TriggerPostLevelChange: No game instance!"));
		return false;
	}

	//
	class APlayerController *pController = UGameplayStatics::GetPlayerController(WorldContext, 0);
	if (!IsValid(pController))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::TriggerPostLevelChange: Failed to get player controller!"));
		return false;
	}

	//
	class APawn *pPlayer = UGameplayStatics::GetPlayerPawn(WorldContext, 0);
	if (!IsValid(pPlayer))
	{
		UE_LOG(LogTemp, Fatal, TEXT("USimpleSaveFile::TriggerPostLevelChange: Failed to get player!"));
		return false;
	}


	class AActor *pActor = pGameInstance->LevelChangeActor.Get();
	if (!IsValid(pActor))
	{
		UE_LOG(LogTemp, Error, TEXT("No level change actor!"));
		return false;
	}

	if (ILevelChangeInterface::Execute_PostLevelChange(pActor, pController, pPlayer))
	{
		pGameInstance->LevelChangeActor = NULL;
		return true;
	}

	return false;
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::OnRestoreActor(const FActorSaveData &InData, class AActor *InActor, bool InGlobal)
{
	OnRestoreObject(InData.Custom, InActor, InGlobal);

	//Restore components as well
	for (int32 i=0; i<InData.Components.Num(); i++)
	{
		class UActorComponent *pComponent = GetComponent(InActor, InData.Components.GetData()[i]);
		if (pComponent)
		{
			OnRestoreObject(InData.Components.GetData()[i].Custom, pComponent, InGlobal);
		}
	}
}

//=================================================================
// 
//=================================================================
class UObject *USimpleSaveFile::OnRestoreObject(const FCustomSaveData &InData, class UObject* InObject, bool InGlobal)
{
	TArray<class UObject*> *SaveObjects = InGlobal ? &GlobalSaveObjects : &LocalSaveObjects;

	if (InData.ObjectIndex >= SaveObjects->Num())
	{
		SaveObjects->SetNum(InData.ObjectIndex+1);
	}

	SaveObjects->GetData()[InData.ObjectIndex] = InObject;
	return InObject;
}

//=================================================================
// 
//=================================================================
class UObject *USimpleSaveFile::GetRestoreObject(const FCustomSaveData &InData, bool InGlobal) const
{
	return GetRestoreObjectIndex(InData.ObjectIndex, InGlobal);
}

//=================================================================
// 
//=================================================================
class UObject *USimpleSaveFile::GetRestoreObjectIndex(int32 ObjectIndex, bool InGlobal) const
{
	const TArray<class UObject*> *SaveObjects = InGlobal ? &GlobalSaveObjects : &LocalSaveObjects;
	if (ObjectIndex >= SaveObjects->Num())
		return NULL;

	return SaveObjects->GetData()[ObjectIndex];
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::ClearActorsOnRestore(class UObject *WorldContext, bool IsLevelChange)
{
	TArray<class AActor*> AllActors;
	UGameplayStatics::GetAllActorsWithInterface(WorldContext, USaveInterface::StaticClass(), AllActors);

	//Go through all the actors
	for (int32 i=AllActors.Num()-1; i>=0; i--)
	{
		//If already saved then ignore
		if (GlobalSaveObjects.Contains(AllActors.GetData()[i]))
		{
			continue;
		}

		//If already saved then ignore
		if (LocalSaveObjects.Contains(AllActors.GetData()[i]))
		{
			continue;
		}

		ISaveInterface *pInterface = Cast<ISaveInterface>(AllActors.GetData()[i]);
		if (!pInterface)
		{
			continue;
		}

		if (pInterface->ShouldDeleteOnRestore() && (!IsLevelChange || !pInterface->ShouldRespawnOnLevelChange()))
		{
#if WITH_EDITOR
			if (GEditor->GetEditorSubsystem<ULayersSubsystem>())
			{
				GEditor->GetEditorSubsystem<ULayersSubsystem>()->DisassociateActorFromLayers(AllActors.GetData()[i]);
			}

			TArray<class AActor*> Children;
			AllActors.GetData()[i]->GetAttachedActors(Children);
			for (int32 j=0; j<Children.Num(); j++)
			{
				Children.GetData()[j]->Destroy();

				UE_LOG(LogTemp, Error, TEXT("USimpleSaveFile::ClearActorsOnRestore: Actor %s had child %s"), *AllActors.GetData()[i]->GetName(), *Children.GetData()[j]->GetName());
			}

			//UE_LOG(LogTemp, Error, TEXT("Destroying actor %s"), *AllActors.GetData()[i]->GetActorLabel());
#endif //

			AllActors.GetData()[i]->Destroy();
			continue;
		}
	}
}

//=================================================================
// 
//=================================================================
class UObject *USimpleSaveFile::GetObjectOuter(FLevelSaveData *InLevelData, const FCustomSaveData &InData) const
{
	if (InData.OuterIsGlobal)
	{
		return GetRestoreObjectIndex(InData.OuterObjectIndex, true);
	}

	return GetRestoreObjectIndex(InData.OuterObjectIndex, false);
}

/*
//=================================================================
// 
//=================================================================
FORCEINLINE const FCustomSaveData *GetOuterDataFromActor(const FActorSaveData &InActorData, const FCustomSaveData &InData)
{
	if (InActorData.Custom.ObjectIndex == InData.OuterObjectIndex)
		return &InActorData.Custom;
}
*/

//=================================================================
// 
//=================================================================
const FCustomSaveData *USimpleSaveFile::GetOuterObjectData(FLevelSaveData *InLevelData, const FCustomSaveData &InData) const
{
	const TArray<FCustomSaveData> &CustomObjectsArray = InData.OuterIsGlobal ? CustomObjects : InLevelData->CustomObjects;
	for (int32 i=0; i<CustomObjectsArray.Num(); i++)
	{
		if (CustomObjectsArray.GetData()[i].ObjectIndex == InData.OuterObjectIndex)
			return &CustomObjectsArray.GetData()[i];
	}

	return NULL;
}

//=================================================================
// 
//=================================================================
class UObject *USimpleSaveFile::RecreateDynamicObject(FLevelSaveData *InLevelData, const FCustomSaveData &InData, bool InGlobal)
{
	//Check if already restored
	class UObject *pObject = USimpleSaveFile::GetRestoreObject(InData, InGlobal);
	if (IsValid(pObject))
		return pObject;

	int32 iIndex = InData.ObjectIndex;

	//Load the class
	int32 iMemory = KeepInMemory.AddUnique(InData.Class.IsPending() ? InData.Class.LoadSynchronous() : InData.Class.Get());
	class UClass *pClass = (UClass*)(KeepInMemory.GetData()[iMemory]);
	if (!pClass)
	{
		UE_LOG(LogTemp, Fatal, TEXT("No class defined on RecreateDynamicObjects for \"%s\" with class \"%s\"!"), *InData.Name.ToString(), *InData.Class.ToString());
		return NULL;
	}

	//
	if (InData.OuterObjectIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Fatal, TEXT("No outer object index defined on RecreateDynamicObjects for \"%s\" with class \"%s\"!"), *InData.Name.ToString(), *InData.Class.ToString());
		return NULL;
	}

	//
	class UObject *pOuter = GetObjectOuter(InLevelData, InData);

	//If outer hasn't been recreated yet, then do it now
	if (!IsValid(pOuter))
	{
		const FCustomSaveData *pOuterData = GetOuterObjectData(InLevelData, InData);
		if (pOuterData)
		{
			pOuter = RecreateDynamicObject(InLevelData, *pOuterData, InData.OuterIsGlobal);
		}
	}

	//
	if (!pOuter)
	{
		UE_LOG(LogTemp, Fatal, TEXT("No outer defined on RecreateDynamicObjects!"));
		return NULL;		
	}

	iMemory = KeepInMemory.Add(NewObject<UObject>(pOuter, pClass));
	return OnRestoreObject(InData, KeepInMemory.GetData()[iMemory], InGlobal);
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::RecreateDynamicObjects(FLevelSaveData *InLevelData, const TArray<FCustomSaveData> &InData, bool InGlobal)
{
	//Go through the array
	for (int32 i=0; i<InData.Num(); i++)
	{
		RecreateDynamicObject(InLevelData, InData.GetData()[i], InGlobal);
	}
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::RespawnOrFindActors(class UObject *WorldContext, const TArray<FActorSaveData> &InData, TArray<class UObject*> &InObjects, bool InGlobal)
{
	class UWorld *pWorld = WorldContext->GetWorld();

	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	//Go through all the actors
	for (int32 i=0; i<InData.Num(); i++)
	{
		const FActorSaveData &MyData = InData.GetData()[i];

		//Check if already restored
		class AActor *pActor = Cast<AActor>(GetRestoreObject(MyData.Custom, InGlobal));
		if (IsValid(pActor))
			continue;

		//Load class
		int32 iMemory = KeepInMemory.AddUnique(MyData.Custom.Class.IsPending() ? MyData.Custom.Class.LoadSynchronous() : MyData.Custom.Class.Get());
		
		TSubclassOf<class UObject> ObjectClass = (UClass *)KeepInMemory.GetData()[iMemory];
		if (!ObjectClass)
		{
			UE_LOG(LogTemp, Fatal, TEXT("Failed to load class \"%s\""), *MyData.Custom.Class.ToString());
			continue;
		}

		//Make sure right type of class
		if (!ObjectClass->IsChildOf(AActor::StaticClass()))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Failed to load actor class \"%s\""), *MyData.Custom.Class.ToString());
			continue;
		}

		//Respawn if needed
		if (MyData.Custom.Recreate)
		{
			pActor = pWorld->SpawnActor(ObjectClass, &MyData.Transform, Parameters);
		}
		else
		{
			TArray<class AActor*> AllActors;
			UGameplayStatics::GetAllActorsOfClass(WorldContext, (TSubclassOf<AActor>)ObjectClass, AllActors);
			for (int32 j=0; j<AllActors.Num(); j++)
			{
				if (!MyData.Custom.Tag.IsNone())
				{
					class ISaveInterface *pInterface = Cast<ISaveInterface>(AllActors.GetData()[j]);
					if (!pInterface)
					{
						continue;
					}

					if (pInterface->GetSavingTag() == MyData.Custom.Tag)
					{
						pActor = AllActors.GetData()[j];
						break;
					}

					continue;
				}
				
				if (AllActors.GetData()[j]->GetFName() == MyData.Custom.Name)
				{
					pActor = AllActors.GetData()[j];
					break;
				}
			}
		}

		//UE_LOG(LogTemp, Display, TEXT("Respawned or found \"%s\" with tag \"%s\""), *MyData.Custom.Name.ToString(), InGlobal ? TEXT("Global") : TEXT("Local"));

		//If somehow failed
		if (!IsValid(pActor))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Failed to respawn or find \"%s\" with tag \"%s\""), *MyData.Custom.Name.ToString(), InGlobal ? TEXT("Global") : TEXT("Local"));
			continue;
		}

		OnRestoreActor(MyData, pActor, InGlobal);
	}
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::RestoreActor(FLevelSaveData *InLevelData, class AActor *InActor, const FActorSaveData &InData)
{
	if (!IsValid(InActor))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Actor not found with name %s"), *InData.Custom.Name.ToString());
		return;
	}

	if (InActor->GetRootComponent() && InActor->GetRootComponent()->Mobility == EComponentMobility::Movable)
	{
		InActor->SetActorLocation(InData.Transform.GetLocation());
		InActor->SetActorRotation(InData.Transform.GetRotation().Rotator());
		InActor->SetActorScale3D(InData.Transform.GetScale3D());
	}

	//Restore actor custom data
	RestoreCustomData(InActor, InData.Custom);

	//Reattach to parent
	if (InData.Custom.OuterObjectIndex != INDEX_NONE)
	{
		class USceneComponent *pParent = Cast<USceneComponent>(GetObjectOuter(InLevelData, InData.Custom));
		if (pParent)
		{
			//UE_LOG(LogTemp, Error, TEXT("%s restoring attach parent %s (%s)"), *InActor->GetName(), *pParent->GetName(), *pParent->GetOwner()->GetName());

			class ISaveInterface *pInterface = Cast<ISaveInterface>(InActor);
			if (pInterface)
			{
				pInterface->HandleReattach(InActor, pParent, InData.AttachSocketName, InData.RelativeTransform);
			}
			else
			{
				InActor->AttachToComponent(pParent, FAttachmentTransformRules::KeepWorldTransform, InData.AttachSocketName);

				//UE_LOG(LogTemp, Warning, TEXT("%s reattaching to %s"), *InActor->GetName(), *pParent->GetOwner()->GetName());
			}
		}
		else
		{
			UE_LOG(LogTemp, Fatal, TEXT("Failed to find attach parent %d for %s of class %s"), InData.Custom.OuterObjectIndex, *InData.Custom.Name.ToString(), *InData.Custom.Class->GetName());
		}
	}

	//Go through all the components
	for (int32 i=0; i<InData.Components.Num(); i++)
	{
		class UActorComponent *pComponent = GetComponent(InActor, InData.Components.GetData()[i]);
		if (pComponent)
		{
			RestoreComponent(InActor, pComponent, InData.Components.GetData()[i]);
		}
	} 
}

//=================================================================
// 
//=================================================================
void ISaveInterface::HandleReattach(class AActor *InActor, class USceneComponent *InComponent, const FName &InSocket, const FTransform &InRelativeTransform)
{
	InActor->AttachToComponent(InComponent, FAttachmentTransformRules::KeepWorldTransform, InSocket);
	InActor->SetActorRelativeTransform(InRelativeTransform);
}

//=================================================================
// 
//=================================================================
class UActorComponent *USimpleSaveFile::GetComponent(class AActor *InActor, const FComponentSaveData &InData) const
{
	//Get components
	TArray<class UActorComponent*> Components;
	InActor->GetComponents(Components);

	for (int32 j=0; j<Components.Num(); j++)
	{
		if (Components.GetData()[j]->GetFName() == InData.Custom.Name)
		{
			return Components.GetData()[j];
		}
	}

	return NULL;
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::RestoreComponent(class AActor *InActor, class UActorComponent *InComponent, const FComponentSaveData &InData)
{
	if (!IsValid(InComponent))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Invalid component with name \"%s\" and tag \"%s\""), *InData.Custom.Name.ToString(), *InData.Custom.Tag.ToString());
		return;
	}

	class USceneComponent *pSceneComponent = Cast<USceneComponent>(InComponent);
	if (pSceneComponent && pSceneComponent != InActor->GetRootComponent() && pSceneComponent->Mobility == EComponentMobility::Movable)
	{
		pSceneComponent->SetRelativeTransform(InData.Transform);
	}

	RestoreCustomData(InComponent, InData.Custom);
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::RestoreCustomData_Internal(class USimpleSaveFile *InFile, class UObject *InObject, const TMap<FName, FString> &Singles, const TMap<FName, FArrayData> &Arrays, const TMap<FName, FMapData> &Maps)
{
	class ISaveInterface *pInterface = Cast<ISaveInterface>(InObject);
	if (pInterface)
	{
		pInterface->PreRestore();
	}

	int32 TextPortFlags = PPF_SimpleObjectText;

	//Go through normal variables
	for (auto It = Singles.CreateConstIterator(); It; ++It)
	{
		class FProperty *Property = FindFProperty<FProperty>(InObject->GetClass(), It.Key());
		if (Property)
		{
			FString Value = It.Value();
			void *ValuePtr = Property->ContainerPtrToValuePtr<void>(InObject, 0);

			if (HandleRestoreObject(InFile, Property, Value, ValuePtr, 0, InObject))
			{
				continue;
			}
			
			if (HandleRestoreStruct(InFile, Property, Value, ValuePtr, 0, InObject)) 
			{
				continue;
			}

			HandleFixSoftObject(InObject, Property, Value);

			Property->ImportText_Direct(*Value, ValuePtr, InObject, TextPortFlags);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find property \"%s\" on object \"%s\""), *It.Key().ToString(), *InObject->GetName());
			continue;
		}
	}

	//Go through maps
	for (auto It = Maps.CreateConstIterator(); It; ++It)
	{
		class FMapProperty *MapProperty = FindFProperty<FMapProperty>(InObject->GetClass(), It.Key());
		if (!MapProperty)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find property \"%s\" on object \"%s\""), *It.Key().ToString(), *InObject->GetName());
			continue;
		}

		const TMap<FString, FString> Map = It.Value().Data;

		FScriptMapHelper_InContainer MapHelper(MapProperty, InObject, 0);
		MapHelper.EmptyValues();

		//
		for (auto MapItr = Map.CreateConstIterator(); MapItr; ++MapItr)
		{
			int32 Index = MapHelper.AddDefaultValue_Invalid_NeedsRehash();

			FString Key = MapItr.Key();

			if (!HandleRestoreObject(InFile, MapProperty->KeyProp, Key, MapHelper.GetKeyPtr(Index), 0, InObject) &&
				!HandleRestoreStruct(InFile, MapProperty->KeyProp, Key, MapHelper.GetKeyPtr(Index), 0, InObject))
			{
				HandleFixSoftObject(InObject, MapProperty->KeyProp, Key);

				MapProperty->KeyProp->ImportText_Direct(*Key, MapHelper.GetKeyPtr(Index), InObject, TextPortFlags);
			}

			FString Value = MapItr.Value();

			if (!HandleRestoreObject(InFile, MapProperty->ValueProp, Value, MapHelper.GetValuePtr(Index), 0, InObject) &&
				!HandleRestoreStruct(InFile, MapProperty->ValueProp, Value, MapHelper.GetValuePtr(Index), 0, InObject))
			{
				HandleFixSoftObject(InObject, MapProperty->ValueProp, Value);

				//UE_LOG(LogTemp, Error, TEXT("%s Restoring property %s value %s"), *InObject->GetClass()->GetName(), *MapProperty->GetName(), *Value);

				MapProperty->ValueProp->ImportText_Direct(*Value, MapHelper.GetValuePtr(Index), InObject, TextPortFlags);
			}	
		}

		MapHelper.Rehash();
	}

	//Go through arrays
	for (auto It = Arrays.CreateConstIterator(); It; ++It)
	{
		class FProperty *Property = FindFProperty<FProperty>(InObject->GetClass(), It.Key());
		if (!Property)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find property \"%s\" on object \"%s\""), *It.Key().ToString(), *InObject->GetName());
			continue;
		}

		const TArray<FString> &Array = It.Value().Data;

		//
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property);
		if (ArrayProperty)
		{
			//void* ValuePtr = Property->ContainerPtrToValuePtr<void>(InObject, 0);
			FScriptArrayHelper_InContainer ArrayHelper(ArrayProperty, InObject);
			//FScriptArrayHelper ArrayHelper(ArrayProperty, ValuePtr);

			ArrayHelper.EmptyAndAddValues(Array.Num());

			for (int32 Index = 0; Index < Array.Num(); Index++)
			{
				FString Value = Array.GetData()[Index];
				
				if (HandleRestoreObject(InFile, ArrayProperty->Inner, Value, ArrayHelper.GetRawPtr(Index), 0, InObject))
					continue;

				if (HandleRestoreStruct(InFile, ArrayProperty->Inner, Value, ArrayHelper.GetRawPtr(Index), 0, InObject))
					continue;

				HandleFixSoftObject(InObject, ArrayProperty->Inner, Value);

				ArrayProperty->Inner->ImportText_Direct(*Value, ArrayHelper.GetRawPtr(Index), InObject, TextPortFlags);
			}

			//UE_LOG(LogTemp, Error, TEXT("Restored array property %s on %s with size %d"), *Property->GetName(), *InObject->GetName(), Array.Num());
			continue;
		}
		
		//Regular variable with []
		if (Property->ArrayDim == Array.Num())
		{
			for (int32 Index = 0; Index < Array.Num(); Index++)
			{
				FString Value = Array.GetData()[Index];
				void *ValuePtr = Property->ContainerPtrToValuePtr<void>(InObject, Index);

				if (!HandleRestoreObject(InFile, Property, Value, ValuePtr, 0, InObject) &&
					!HandleRestoreStruct(InFile, Property, Value, ValuePtr, 0, InObject))
				{
					HandleFixSoftObject(InObject, Property, Value);

					Property->ImportText_Direct(*Value, ValuePtr, InObject, TextPortFlags);
				}
			}
		}
	}
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::FinishLoading()
{
	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
	LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("Finishing restoration...")));

	GlobalSaveObjects.Reset();
	LocalSaveObjects.Reset();
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::ClearCurrentLevelAndGlobalData(class UObject *WorldContextObject)
{
	FName MapName = *UGameplayStatics::GetCurrentLevelName(WorldContextObject);

	ClearLevelData(MapName);

	CustomObjects.Reset();
	GlobalActors.Reset();
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::ClearLevelData(const FName &InLevel)
{
	for (int32 i=Levels.Num()-1; i>=0; i--)
	{
		if (Levels.GetData()[i].LevelName == InLevel)
		{
			Levels.RemoveAt(i);
			return true;
		}
	}

	return false;
}

//==============================================================================================================
//
//==============================================================================================================
bool USimpleSaveFile::StripLevelNameString(const FString& InString, FString& OutString)
{
	if (InString.Split(TEXT("."), NULL, &OutString, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
	{
		return true;
	}

	if (InString.Split(TEXT("/"), NULL, &OutString, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
	{
		return true;
	}

	OutString = InString;
	return false;
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::SetCurrentMapName(const class UObject * const InObject)
{
	FString MapName = InObject->GetWorld()->GetOutermost()->GetName();
	MapName = InObject->GetWorld()->RemovePIEPrefix(MapName);
	StripLevelNameString(MapName, MapName);
	CurrentMapName = *MapName;
}

//=================================================================
// 
//=================================================================
FLevelSaveData *USimpleSaveFile::SaveLevelData(class USaveGameInstance *InInstance, class APlayerController *InController, class APawn *InPawn, const class UObject *WorldContext)
{
	FName MapName = *UGameplayStatics::GetCurrentLevelName(WorldContext);

	FLevelSaveData *pLevelData = NULL;
	for (int32 i=0; i<Levels.Num(); i++)
	{
		if (Levels.GetData()[i].LevelName == MapName)
		{
			pLevelData = &Levels.GetData()[i];
			break;
		}
	}

	//If no level data
	if (pLevelData)
	{
		pLevelData->Actors.Reset();
	}
	else
	{
		FLevelSaveData NewLevelData;
		NewLevelData.LevelName = MapName;
		int32 i = Levels.Add(NewLevelData);
		pLevelData = &Levels.GetData()[i];
	}

	pLevelData->CustomObjects.Reset();
	pLevelData->Actors.Reset();

	TMap<FName, class AActor*> CustomTags;
	InInstance->GetLocalActorTags(InController, InPawn, CustomTags);
	for (auto It = CustomTags.CreateConstIterator(); It; ++It)
	{
		AddActorToSave(pLevelData, It.Value(), It.Key());
	}

	SetCurrentMapName(WorldContext);

	TArray<class AActor*> AllActors;
	UGameplayStatics::GetAllActorsWithInterface(WorldContext, USaveInterface::StaticClass(), AllActors);

	//Go through all the actors
	for (int32 i=0; i<AllActors.Num(); i++)
	{
		if (IsMarkedForDestruction(AllActors.GetData()[i]))
		{
			continue;
		}

		//If already saved then ignore
		if (GlobalSaveObjects.Contains(AllActors.GetData()[i]))
		{
			//AllActors.RemoveAt(i);
			continue;
		}

		if (LocalSaveObjects.Contains(AllActors.GetData()[i]))
		{
			//AllActors.RemoveAt(i);
			continue;
		}

		ISaveInterface *pInterface = Cast<ISaveInterface>(AllActors.GetData()[i]);
		if (!pInterface)
		{
			//AllActors.RemoveAt(i);
			continue;
		}

		if (!pInterface->ShouldSave())
		{
			//AllActors.RemoveAt(i);
			continue;
		}

		AddActorToSave(pLevelData, AllActors.GetData()[i], NAME_None);

		/*
		UE_LOG(LogTemp, Error, TEXT("Saving %s with local index %d global index %d in %s"),
						*AllActors.GetData()[i]->GetName(), 
						LocalSaveObjects.Find(AllActors.GetData()[i]),
						GlobalSaveObjects.Find(AllActors.GetData()[i]), 
						*GetName());		
						*/

		//LocalSaveObjects.Add(AllActors.GetData()[i]);
	}

	/*
	//Now save the actors
	for (int32 i=0; i<AllActors.Num(); i++)
	{
		FActorSaveData ActorData;
		if (SaveActor(AllActors.GetData()[i], ActorData, true))
		{
			pLevelData->Actors.Add(ActorData);
		}
	}
	*/

	//Now save all objects that haven't been saved yet
	//SaveDynamicObjects(LocalSaveObjects, pLevelData->CustomObjects, true);
	return pLevelData;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::SaveActor(class AActor *InActor, FActorSaveData &InData)
{
	if (!IsValid(InActor))
	{
		UE_LOG(LogTemp, Fatal, TEXT("%s is not an actor!"), *InData.Custom.Name.ToString());
		return false;
	}
	
	InData.Transform = InActor->GetActorTransform();
	
	SaveCustomData(InActor, InData.Custom);

	for (int32 i=0; i<InData.Components.Num(); i++)
	{
		class UActorComponent *pComponent = GetComponent(InActor, InData.Components.GetData()[i]);
		SaveComponent(pComponent, InData.Components.GetData()[i]);
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::SaveComponent(class UActorComponent *InComponent, FComponentSaveData &InData)
{
	if (!IsValid(InComponent))
	{
		UE_LOG(LogTemp, Fatal, TEXT("%s is not a component!"), *InData.Custom.Name.ToString());
		return false;
	}

	SaveCustomData(InComponent, InData.Custom);

	class USceneComponent *pScene = Cast<USceneComponent>(InComponent);
	if (pScene)
	{
		InData.Transform = pScene->GetRelativeTransform();
	}

	return true;
}

//=================================================================
// 
//=================================================================
static bool CheckSaveObjectProperty(const FProperty *InProperty, void *InRawData)
{
	const FObjectPropertyBase *ObjectProperty = CastField<FObjectPropertyBase>(InProperty);
	if (!ObjectProperty)
		return true;

	const FSoftObjectProperty *SoftObjectProperty = CastField<FSoftObjectProperty>(InProperty);
	if (SoftObjectProperty)
		return true;

	if (!InRawData)
		return false;

	class UObject *pObject = ObjectProperty->GetObjectPropertyValue(InRawData);
	if (!IsValid(pObject))
		return false;

	if (!USimpleSaveFile::IsValidDynamicObject(pObject))
		return true;

	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleSaveStruct(class USimpleSaveFile *InFile, FProperty *InProperty, class UObject *InObject, void *InRawData, FString& OutString)
{
	//Special handling for structs
	FStructProperty* StructProperty = CastField<FStructProperty>(InProperty);
	if (!StructProperty)
		return false;

	if (StructProperty->Struct == FSavedTime::StaticStruct())
	{
		FSavedTime *ValuePtr = (FSavedTime*)InRawData;
	
		float flTime = 0.0f;
		if (ValuePtr->Time != 0.0f)
		{
			flTime = ValuePtr->Time - UGameplayStatics::GetTimeSeconds(InObject);
		}

		//UE_LOG(LogTemp, Error, TEXT("Saving time %.2f"), flTime);

		OutString = FString::Printf(TEXT("%f"), flTime);
		return true;
	}

	int32 PortFlags = PPF_SimpleObjectText;

	OutString = TEXT("(");

	//
	int nCount = 0;
	bool bHadObject = false;
	bool bSaveAll = StructProperty->Struct == FDataTableRowHandle::StaticStruct();

	//Check if we should save all
	if (!bSaveAll)
	{
		bool bHasSaveFlags = false;
		
		//
		for (TFieldIterator<FProperty> Property(StructProperty->Struct); Property; ++Property)
		{
			//
			if ((Property->GetPropertyFlags() & CPF_SaveGame) == 0)
				continue;

			bHasSaveFlags = true;
			break;
		}

		//Save all if no flags
		if (!bHasSaveFlags)
		{
			bSaveAll = true;
		}
	}

	//
	for (TFieldIterator<FProperty> Property(StructProperty->Struct); Property; ++Property)
	{
		//
		if (!bSaveAll && (Property->GetPropertyFlags() & CPF_SaveGame) == 0)
			continue;

		void *ValuePtr = Property->ContainerPtrToValuePtr<void>(InRawData);

		if (!InFile && !CheckSaveObjectProperty(InProperty, ValuePtr))
		{
			//UE_LOG(LogTemp, Error, TEXT("USimpleSaveFile: Couldn't save property \"%s\""), *Property->GetName());
			continue;
		}


		if (nCount > 0)
		{
			OutString += TEXT(",");
		}

		nCount++;

		OutString += Property->GetName() + TEXT("=");

		FString SaveString;
		if (HandleSaveObject(InFile, *Property, InObject, ValuePtr, 0, SaveString))
		{
			//UE_LOG(LogTemp, Error, TEXT("Saved object inside struct!"));
			bHadObject = true;
		}
		else
		{
			Property->ExportTextItem_Direct(SaveString, ValuePtr, ValuePtr, InObject, PortFlags);
		}

		OutString += SaveString;
	}

	OutString += TEXT(")");

	//UE_LOG(LogTemp, Error, TEXT("Saving struct \"%s\" as \"%s\""), *StructProperty->GetName(), *OutString);
	return true;
}

//=================================================================
// 
//=================================================================
class FObjectPropertyBase *USimpleSaveFile::CastToObjectProperty(class FProperty *InProperty)
{
	class FObjectProperty *ObjectProperty = CastField<FObjectProperty>(InProperty);
	if (ObjectProperty)
		return ObjectProperty;

	class FWeakObjectProperty *WeakObjectProperty = CastField<FWeakObjectProperty>(InProperty);
	if (WeakObjectProperty)
		return WeakObjectProperty;

	return NULL;
}

//=================================================================
// 
//=================================================================
bool LogSaveAsSoftObject(class UObject *InObject)
{
	if (InObject->GetClass()->IsChildOf(UActorComponent::StaticClass()))
		return true;

	if (InObject->GetClass()->IsChildOf(AActor::StaticClass()))
		return true;

	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::ForceSaveAsSoftObject(class UObject *InObject)
{
	if (InObject->GetClass()->IsChildOf(UActorComponent::StaticClass()))
		return true;

	if (InObject->GetClass()->IsChildOf(AActor::StaticClass()))
		return true;

	if (InObject->GetClass()->IsChildOf(UVisual::StaticClass()))
		return true;

	if (InObject->GetClass()->IsChildOf(UDataAsset::StaticClass()))
		return true;

	if (InObject->GetClass()->IsChildOf(UStreamableRenderAsset::StaticClass()))
		return true;

	if (InObject->GetClass() == UDataTable::StaticClass())
		return true;

	if (InObject->GetClass()->IsChildOf(UClass::StaticClass()))
		return true;

	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::IsValidDynamicObject(class UObject *InObject)
{
	if (InObject->HasAnyFlags(RF_Standalone))
		return false;

	if (InObject->GetClass()->IsChildOf(UWorld::StaticClass()))
		return false;

	if (ForceSaveAsSoftObject(InObject))
		return false;

	if (InObject->GetClass()->IsChildOf(UDataAsset::StaticClass()))
		return false;

	if (InObject->GetClass() == UBlueprintGeneratedClass::StaticClass())
		return false;

	if (InObject->GetClass()->IsChildOf(UVisual::StaticClass()))
		return false;

	if (InObject->GetClass()->IsChildOf(UStreamableRenderAsset::StaticClass()))
		return false;

	if (InObject->GetClass() == UDataTable::StaticClass())
		return false;

	if (InObject->GetClass()->IsChildOf(UAnimInstance::StaticClass()))
		return false;

	return true;
}

//=================================================================
// 
//=================================================================
FString USimpleSaveFile::GetSavingObjectPrefix(bool InGlobal) const
{
	static const FString String_Global = TEXT("![Global]:");
	return InGlobal ? String_Global : FString::Printf(TEXT("!%s:"), *CurrentMapName.ToString());
}

//=================================================================
// 
//=================================================================
FString USimpleSaveFile::GetSavingObjectString(bool InGlobal, const FName &InTag, int32 InIndex) const
{
	return FString::Printf(TEXT("%s%d"), *GetSavingObjectPrefix(InGlobal), InIndex);
}

//=================================================================
// 
//=================================================================
FORCEINLINE static bool DoesReferenceString(const FCustomSaveData &InData, const FString &InString)
{
	for (auto It = InData.Singles.CreateConstIterator(); It; ++It)
	{
		if (It.Value().Equals(InString))
			return true;
	}

	for (auto It = InData.Arrays.CreateConstIterator(); It; ++It)
	{
		for (int32 i=0; i<It.Value().Data.Num(); i++)
		{
			if (It.Value().Data.GetData()[i].Equals(InString))
				return true;
		}
	}

	for (auto It = InData.Maps.CreateConstIterator(); It; ++It)
	{
		for (auto It2 = It.Value().Data.CreateConstIterator(); It2; ++It2)
		{
			if (It2.Key().Equals(InString) || It2.Value().Equals(InString))
				return true;
		}
	}

	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::FindFirstReferenceTo(FString InString, FString& OutObjectThatReferences)
{
	for (int32 i=0; i<GlobalActors.Num(); i++)
	{
		const FActorSaveData &ActorData = GlobalActors.GetData()[i];

		if (DoesReferenceString(ActorData.Custom, InString))
		{
			OutObjectThatReferences = GetSavingObjectString(true, ActorData.Custom.Tag, ActorData.Custom.ObjectIndex);
			return true;
		}

		//Go through components
		for (int32 j=0; j<ActorData.Components.Num(); j++)
		{
			const FComponentSaveData &ComponentData = ActorData.Components.GetData()[j];

			if (DoesReferenceString(ComponentData.Custom, InString))
			{
				OutObjectThatReferences = GetSavingObjectString(true, ActorData.Custom.Tag, ActorData.Custom.ObjectIndex) + TEXT(" - ") + GetSavingObjectString(true, ComponentData.Custom.Tag, ComponentData.Custom.ObjectIndex);
				return true;
			}
		}
	}

	for (int32 i=0; i<CustomObjects.Num(); i++)
	{
		const FCustomSaveData & Data = CustomObjects.GetData()[i];

		if (DoesReferenceString(Data, InString))
		{
			OutObjectThatReferences = GetSavingObjectString(true, Data.Tag, Data.ObjectIndex);
			return true;
		}
	}

	//Go through all the levels
	for (int32 k=0; k<Levels.Num(); k++)
	{
		const FLevelSaveData &LevelData = Levels.GetData()[k];

		for (int32 i = 0; i < LevelData.Actors.Num(); i++)
		{
			const FActorSaveData& ActorData = LevelData.Actors.GetData()[i];

			if (DoesReferenceString(ActorData.Custom, InString))
			{
				OutObjectThatReferences = GetSavingObjectString(false, ActorData.Custom.Tag, ActorData.Custom.ObjectIndex);
				return true;
			}

			//Go through components
			for (int32 j = 0; j < ActorData.Components.Num(); j++)
			{
				const FComponentSaveData& ComponentData = ActorData.Components.GetData()[j];

				if (DoesReferenceString(ComponentData.Custom, InString))
				{
					OutObjectThatReferences = GetSavingObjectString(false, ActorData.Custom.Tag, ActorData.Custom.ObjectIndex) + TEXT(" - ") + GetSavingObjectString(false, ComponentData.Custom.Tag, ComponentData.Custom.ObjectIndex);
					return true;
				}
			}
		}

		for (int32 i = 0; i < LevelData.CustomObjects.Num(); i++)
		{
			const FCustomSaveData& Data = LevelData.CustomObjects.GetData()[i];

			if (DoesReferenceString(Data, InString))
			{
				OutObjectThatReferences = GetSavingObjectString(false, Data.Tag, Data.ObjectIndex);
				return true;
			}
		}
	}

	return false;
}

//=================================================================
// 
//=================================================================
const FString &USimpleSaveFile::GetSoftObjectPrefix() 
{
	static const FString String_SoftObject = TEXT("![SoftObject]:");
	return String_SoftObject;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::GetSoftObjectString(class UObject *InObject, FString &OutString)
{
	TSoftObjectPtr<class UObject> SoftObject(InObject);
	OutString = FString::Printf(TEXT("%s%s"), *GetSoftObjectPrefix(), *SoftObject.ToString());
	return true;
}

//=================================================================
// 
//=================================================================
FORCEINLINE static bool ShouldSaveIntoAssetArray(class UObject *InObject)
{
	if (InObject->GetClass()->IsChildOf(UDataAsset::StaticClass()))
		return true;

	if (InObject->GetClass()->IsChildOf(UVisual::StaticClass()))
		return true;

	if (InObject->GetClass()->IsChildOf(UStreamableRenderAsset::StaticClass()))
		return true;

	if (InObject->GetClass() == UDataTable::StaticClass())
		return true;

	if (InObject->GetClass()->IsChildOf(UAnimInstance::StaticClass()))
		return true;

	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleSaveObject_Internal(class FProperty *InProperty, class UObject *InObject, void *InRawData, int32 InIndex, FString &OutString)
{
	//Check that object property
	class FObjectPropertyBase *ObjectProperty = CastField<FObjectPropertyBase>(InProperty); //CastToObjectProperty(InProperty);
	if (!ObjectProperty)
	{
		return false;
	}

#if WITH_EDITOR
	bool bDebug = false; //InProperty->GetFName() == TEXT("CarriedItem");
#endif //

	class UObject *pObject = ObjectProperty->GetObjectPropertyValue(InRawData);
	if (!IsValid(pObject))
	{
		//UE_LOG(LogTemp, Error, TEXT("No object for property %s in object %s"), *InProperty->GetName(), *InObject->GetName());
		return false;
	}

	//
	if (CastField<FSoftObjectProperty>(InProperty) == NULL && ShouldSaveIntoAssetArray(pObject))
	{
		AssetsToLoad.AddUnique(pObject);
	}

	FName Tag;
	int32 iIndex;
	bool bGlobal;
	if (GetObjectTag(pObject, Tag, iIndex, bGlobal))
	{
		OutString = GetSavingObjectString(bGlobal, Tag, iIndex);


#if WITH_EDITOR
		if (bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("HandleSaveObject_Internal [%s] Saving Property \"%s\" with object \"%s\" as \"%s\" from tag \"%s\""), *InObject->GetName(), *ObjectProperty->GetName(), *pObject->GetName(), *OutString, *Tag.ToString());
		}
#endif
		return true;
	}

	//Force saving as soft object
	/*
	if (ForceSaveAsSoftObject(pObject) && GetSoftObjectString(pObject, OutString))
	{
		return true;
	}
	*/

	FCustomSaveData *pCurrent = FindObjectToSave(CurrentLevelData, pObject, bGlobal);
	if (pCurrent)
	{
		OutString = GetSavingObjectString(bGlobal, pCurrent->Tag, pCurrent->ObjectIndex);  
		
#if WITH_EDITOR
		if (bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("HandleSaveObject_Internal [%s] Saving Property \"%s\" with object \"%s\" as \"%s\""), *InObject->GetName(), *ObjectProperty->GetName(),  *pObject->GetName(), *OutString);
		}
#endif
		return true;
	}

	//
	/*
	if (ForceSaveAsSoftObject(pObject) && GetSoftObjectString(pObject, OutString))
	{
		if (LogSaveAsSoftObject(pObject))
		{
			UE_LOG(LogTemp, Error, TEXT("Saving \"%s\" as \"%s\""), *pObject->GetName(), *OutString);
		}
		return true;
	}
	*/

	if (ForceSaveAsSoftObject(pObject))
	{
#if WITH_EDITOR
		if (bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("HandleSaveObject_Internal [%s] Saving Property \"%s\" with object \"%s\" should be saved as soft object!"), *InObject->GetName(), *ObjectProperty->GetName(),  *pObject->GetName());
		}
#endif//
		return false;
	}

	//Check if we should save this object as dynamic
	if (!IsValidDynamicObject(pObject))
	{
#if WITH_EDITOR
		if (bDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("HandleSaveObject_Internal [%s] Property \"%s\" Saving \"%s\" of type %s is not valid dynamic object!"), 
			*InObject->GetName(),
			*ObjectProperty->GetName(),
			*pObject->GetName(),
			*pObject->GetClass()->GetName());
		}
	#endif //
		return false;
	}

	bool bObjectIsGlobal = false;
	if (!CurrentLevelData)
	{
		bObjectIsGlobal = true;
	}
	else if (GetObjectTag(pObject->GetOuter(), Tag, iIndex, bGlobal))
	{
		bObjectIsGlobal = bGlobal;

		//UE_LOG(LogTemp, Error, TEXT("Outer found for object %s with %s and index %d global %s"), *pObject->GetName(), *pObject->GetOuter()->GetName(), iIndex, bGlobal ? TEXT("YES") : TEXT("NO"));
	}
	/*
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Outer not found for object %s with %s"), *pObject->GetName(), *pObject->GetOuter()->GetName());
	}
	*/

#if WITH_EDITOR
	int32 iBeforeGlobal = GetGlobalSaveObjects().Num();
	int32 iBeforeLocal = GetLocalSaveObjects().Num();
#endif //

	FCustomSaveData *pData = AddObjectToSave(bObjectIsGlobal ? NULL : CurrentLevelData, pObject, NAME_None);
	if (pData == nullptr)
	{
		UE_LOG(LogTemp, Fatal, TEXT("Failed to AddObjectToSave!"));
		return false;
	}

#if WITH_EDITOR
	if (GetGlobalSaveObjects().Num() > iBeforeGlobal)
	{
		UE_LOG(LogTemp, Error, TEXT("Added global save object \"%s\" referenced by object \"%s\""), 
		*pObject->GetName(),
		*InObject->GetName());
	}

	if (GetLocalSaveObjects().Num() > iBeforeLocal)
	{
		UE_LOG(LogTemp, Error, TEXT("Added local save object \"%s\" referenced by object \"%s\""),
			*pObject->GetName(),
			*InObject->GetName());
	}
#endif //

	//Save a new object
	OutString = GetSavingObjectString(bObjectIsGlobal, pData->Tag, pData->ObjectIndex);  

#if WITH_EDITOR
	if (bDebug)
	{
		UE_LOG(LogTemp, Error, TEXT("HandleSaveObject_Internal [%s] Object \"%s\" with outer \"%s\" saving with tag \"%s\""), *InObject->GetName(), *pObject->GetName(), *pObject->GetOuter()->GetName(), *pData->Tag.ToString());
	}
#endif 
	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestoreStruct(class USimpleSaveFile *InFile, class FProperty *InProperty, const FString &InValue, void *InRawData, int32 InIndex, class UObject *InObject)
{
	//Special handling for structs
	FStructProperty* StructProperty = CastField<FStructProperty>(InProperty);
	if (!StructProperty)
		return false;

	if (StructProperty->Struct == FSavedTime::StaticStruct())
	{
		FSavedTime *ValuePtr = (FSavedTime*)InRawData;
		
		//StructProperty->ContainerPtrToValuePtr<FSavedTime>(InRawData);

		float flTime = FCString::Atof(*InValue);
		if (flTime != 0.0f)
		{
			flTime = UGameplayStatics::GetTimeSeconds(InObject) + flTime;
		}

		//UE_LOG(LogTemp, Error, TEXT("Restored time %.2f"), flTime);

		ValuePtr->Time = flTime;

		//OutString = FString::Printf(TEXT("%f"), flTime);
		return true;
	}

	int32 iLen = InValue.Len();
	if (iLen <= 2)
		return false;

	//Must start with (
	if (InValue[0] != L'(')
		return false;

	if (InValue[iLen-1] != L')')
		return false;

	int32 PortFlags = PPF_SimpleObjectText;

	FString NewValue = InValue.Mid(1, iLen-2);
	iLen = iLen-2;

	TMap<FName, FString> KeyAndValue;
	bool bParsingKey = true;

	FString CurrentKey;
	FString CurrentValue;
	int32 iParenthesis = 0;

	//Go through all the whole value
	for (int32 i=0; i<iLen; i++)
	{
		//If we need to stop parsing the key
		if (bParsingKey && NewValue[i] == L'=')
		{
			bParsingKey = false;
			continue;
		}

		//If we need to stop parsing the value
		if (!bParsingKey && iParenthesis == 0 && NewValue[i] == L',')
		{
			if (CurrentKey.Len() > 0)
			{
				KeyAndValue.Emplace(*CurrentKey, CurrentValue);
			}

			CurrentKey = TEXT("");
			CurrentValue = TEXT("");
			bParsingKey = true;
			continue;
		}

		if (!bParsingKey && NewValue[i] == L'(')
		{
			iParenthesis++;
		}

		if (!bParsingKey && NewValue[i] == L')')
		{
			iParenthesis--;
		}

		if (bParsingKey)
		{
			CurrentKey += NewValue[i];
		}
		else
		{
			CurrentValue += NewValue[i];
		}
	}

	if (CurrentKey.Len() > 0)
	{
		KeyAndValue.Emplace(*CurrentKey, CurrentValue);
	}

	//
	for (auto It = KeyAndValue.CreateConstIterator(); It; ++It)
	{
		class FProperty *Property = FindFProperty<FProperty>(StructProperty->Struct, It.Key());
		if (Property)
		{
			FString Value = It.Value();
			void *ValuePtr = Property->ContainerPtrToValuePtr<void>(InRawData);

			if (HandleRestoreObject(InFile, Property, Value, ValuePtr, 0, InObject))
			{
				continue;
			}

			HandleFixSoftObject(InObject, InProperty, Value);

			Property->ImportText_Direct(*Value, ValuePtr, InObject, PortFlags);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failing to restore key \"%s\" value \"%s\" in struct \"%s\""), *It.Key().ToString(), *It.Value(), *StructProperty->Struct->GetName());
		}
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleFixSoftObject(class UObject *InObject, class FProperty *InProperty, FString &InValue) 
{
	if (InValue.Len() == 0)
		return false;

	if (InProperty != NULL)
	{
		FObjectPropertyBase *pBase = CastField<FObjectPropertyBase>(InProperty);
		if (!pBase)
			return false;

			
		FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(InProperty);
		/*
		if (!SoftObjectProperty && pBase->PropertyClass != NULL && pBase->PropertyClass->IsChildOf(AActor::StaticClass()))
		{
			UE_LOG(LogTemp, Error, TEXT("Hard reference to actor %s in property %s"), *InValue, *InProperty->GetName());
		}
		*/

		/*
		FSoftObjectProperty *SoftObjectProperty = CastField<FSoftObjectProperty>(InProperty);
		if (!SoftObjectProperty)
			return false;
			*/
		
	}


	FString ObjectPointer;
	if (InValue.Split(TEXT(":"), NULL, &ObjectPointer, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		InValue = InObject->GetWorld()->GetPathName() + TEXT(":") + ObjectPointer;
		return true;
	}

	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::HandleRestoreObject_Internal(class FProperty *InProperty, const FString &InValue, void *InRawData, int32 InIndex, class UObject *InObject)
{
	//Check that object property
	class FObjectPropertyBase *ObjectProperty = CastField<FObjectPropertyBase>(InProperty); // CastToObjectProperty(InProperty);
	if (!ObjectProperty)
	{
		return false;
	}

	FString Number;
	bool bSuccess = false;
	bool bGlobal = false;
	if (InValue.Split(GetSavingObjectPrefix(true), NULL, &Number))
	{
		bSuccess = true;
		bGlobal = true;
	}
	else if (InValue.Split(GetSavingObjectPrefix(false), NULL, &Number))
	{
		bSuccess = true;
		bGlobal = false;
	}
	else if (InValue.Split(GetSoftObjectPrefix(), NULL, &Number))
	{
		HandleFixSoftObject(InObject, NULL, Number);

		/*
		const FSoftObjectProperty *SoftObjectProperty = CastField<FSoftObjectProperty>(InProperty);
		if (SoftObjectProperty)
		{
			int32 TextPortFlags = PPF_SimpleObjectText;
			SoftObjectProperty->ImportText_Direct(*Number, InRawData, InObject, TextPortFlags); 
			return true;
		}
		*/
		
		FSoftObjectPath Path(Number);
		TSoftObjectPtr<class UObject> SoftObject(Path);

		int32 iMemory = KeepInMemory.AddUnique(SoftObject.IsPending() ? SoftObject.LoadSynchronous() : SoftObject.Get());
		if (!IsValid(KeepInMemory.GetData()[iMemory]))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Failed to restore from \"%s\""), *Number);
		}
		
		ObjectProperty->SetObjectPropertyValue(InRawData, KeepInMemory.GetData()[iMemory] );
		return true;
	}
	/*
	else if (InValue.Len() > 0 && InValue[0] == L'!' && InProperty->GetFName() == TEXT("Character"))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Property \"%s\" with value \"%s\" and wanted prefix \"%s\" in object \"%s\""), 
		*InProperty->GetName(), 
		*InValue, 
		*GetSavingObjectPrefix(false),
		*InObject->GetName());
	}
	*/

#if WITH_EDITOR
	bool bDebug = false; //ObjectProperty->GetFName() == TEXT("CarriedItem");
#endif //

	//If we succeeded
	if (bSuccess && Number.Len() > 0)
	{
		const TArray<class UObject*> &SaveObjects = bGlobal ? GlobalSaveObjects : LocalSaveObjects;

		int32 iIndex = FCString::Atoi(*Number);
		if (!SaveObjects.IsValidIndex(iIndex))
		{
#if WITH_EDITOR
			if (bDebug)
			{
				UE_LOG(LogTemp, Error, TEXT("HandleRestoreObject_Internal [%s] Property \"%s\" Invalid index %d"), *InObject->GetName(), *ObjectProperty->GetName(), iIndex);
			}
#endif //
			return false;
		}

		//Sanity check
		if (!IsValid(SaveObjects.GetData()[iIndex]))
		{
#if WITH_EDITOR
			if (bDebug)
			{
				UE_LOG(LogTemp, Error, TEXT("HandleRestoreObject_Internal [%s] Property \"%s\" NULL object on index %d"), *InObject->GetName(), *ObjectProperty->GetName(), iIndex);
			}
#endif //
			return false;
		}

		//Sanity check
		if (SaveObjects.GetData()[iIndex]->GetClass() == NULL)
		{
#if WITH_EDITOR
			if (bDebug)
			{
				UE_LOG(LogTemp, Error, TEXT("HandleRestoreObject_Internal [%s] Property \"%s\" Null class for %s"), *InObject->GetName(), *ObjectProperty->GetName(), *ObjectProperty->PropertyClass->GetName());
			}
#endif //
			return false;
		}
		
		//Check that correct class
		if (!SaveObjects.GetData()[iIndex]->GetClass()->IsChildOf(ObjectProperty->PropertyClass))
		{
#if WITH_EDITOR
			if (bDebug)
			{
				UE_LOG(LogTemp, Error, TEXT("HandleRestoreObject_Internal [%s] Property \"%s\" Invalid class %s does not match %s"), *InObject->GetName(), *ObjectProperty->GetName(), *SaveObjects.GetData()[iIndex]->GetClass()->GetName(), *ObjectProperty->PropertyClass->GetName());
			}
#endif //
			return false;
		}

		//Restore the value
		ObjectProperty->SetObjectPropertyValue(InRawData, SaveObjects.GetData()[iIndex]);

#if WITH_EDITOR
		if (bDebug)
		{
			UE_LOG(LogTemp, Display, TEXT("HandleRestoreObject_Internal [%s] Restoring Property \"%s\" to \"%s\""), *InObject->GetName(), *ObjectProperty->GetName(), *SaveObjects.GetData()[iIndex]->GetName());
		}
#endif //
		return true;
	}

	return false;
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::SaveCustomData_Internal(class USimpleSaveFile *InFile, class UObject *InObject, TMap<FName, FString> &Singles, TMap<FName, FArrayData> &Arrays, TMap<FName, FMapData> &Maps, bool SimplePropertiesOnly, TArray<FName> *IgnoredProperties, bool InIgnoreNativeProperties)
{
#if WITH_EDITOR
	bool bDebug = false; //InObject->GetClass()->GetName().Contains(TEXT("Dialogue")) || InObject->GetClass()->GetName().Contains(TEXT("BP_Sawyer"));
#endif 

	Singles.Reset();
	Arrays.Reset();
	Maps.Reset();

	int32 TextPortFlags = PPF_SimpleObjectText;

	//Go through all the properties
	for (TFieldIterator<FProperty> Property(InObject->GetClass()); Property; ++Property)
	{
		//
		if ((Property->GetPropertyFlags() & CPF_SaveGame) == 0)
			continue;

		if (IgnoredProperties != NULL && IgnoredProperties->Contains(Property->GetFName()))
		{
			//UE_LOG(LogTemp, Error, TEXT("Property %s is ignored!"), *Property->GetName());
			continue;
		}

		if (InIgnoreNativeProperties && Property->IsNative())
		{
			//UE_LOG(LogTemp, Error, TEXT("Property %s is ignored as native!"), *Property->GetName());
			continue;
		}

		//Handle arrays separately
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(*Property);
		if (ArrayProperty)
		{
			if (SimplePropertiesOnly)
				continue;

			//Check can save
			if (!InFile && !CheckSaveObjectProperty(ArrayProperty->Inner, NULL))
				continue;

			FArrayData NewArray;
			NewArray.Data.SetNum(0);

			FScriptArrayHelper_InContainer ArrayHelper(ArrayProperty, InObject);
			for (int32 i = 0; i < ArrayHelper.Num(); i++)
			{
				FString NewElement;
				if (!HandleSaveObject(InFile, ArrayProperty->Inner, InObject, ArrayHelper.GetRawPtr(i), 0, NewElement) && !HandleSaveStruct(InFile, ArrayProperty->Inner, InObject, ArrayHelper.GetRawPtr(i), NewElement))
				{
					ArrayProperty->Inner->ExportTextItem_Direct(NewElement, ArrayHelper.GetRawPtr(i), ArrayHelper.GetRawPtr(i), InObject, TextPortFlags);		
				}

				NewArray.Data.Add(NewElement);
			}

			Arrays.Emplace(Property->GetFName(), NewArray);
			continue;
		}

		//Handle maps separately
		FMapProperty* MapProperty = CastField<FMapProperty>(*Property);
		if (MapProperty)
		{
			if (SimplePropertiesOnly)
				continue;

			if (!InFile && !CheckSaveObjectProperty(MapProperty->GetKeyProperty(), NULL))
				continue;

			if (!InFile && !CheckSaveObjectProperty(MapProperty->GetValueProperty(), NULL))
				continue;

			FMapData NewMap;
			NewMap.Data.Empty();

			FScriptMapHelper_InContainer MapHelper(MapProperty, InObject, 0); 

			for (int32 SparseElementIndex = 0; SparseElementIndex < MapHelper.GetMaxIndex(); ++SparseElementIndex)
			{
				if (MapHelper.IsValidIndex(SparseElementIndex))
				{
					FString Key;
					FString Value;

					//Save key
					if (!HandleSaveObject(InFile, MapHelper.GetKeyProperty(), InObject, MapHelper.GetKeyPtr(SparseElementIndex), 0, Key) &&
						!HandleSaveStruct(InFile, MapHelper.GetKeyProperty(), InObject, MapHelper.GetKeyPtr(SparseElementIndex), Key))
					{
						MapHelper.GetKeyProperty()->ExportTextItem_Direct(Key, MapHelper.GetKeyPtr(SparseElementIndex), MapHelper.GetKeyPtr(SparseElementIndex), InObject, TextPortFlags);
					}

					//Save value
					if (!HandleSaveObject(InFile, MapHelper.GetValueProperty(), InObject, MapHelper.GetValuePtr(SparseElementIndex), 0, Value) &&
						!HandleSaveStruct(InFile, MapHelper.GetValueProperty(), InObject, MapHelper.GetValuePtr(SparseElementIndex), Value))
					{
						MapHelper.GetValueProperty()->ExportTextItem_Direct(Value, MapHelper.GetValuePtr(SparseElementIndex), MapHelper.GetValuePtr(SparseElementIndex), InObject, TextPortFlags);
					}

					NewMap.Data.Emplace( Key, Value );		
				}
			}

			Maps.Emplace(Property->GetFName(), NewMap);
			continue;
		}

		if (SimplePropertiesOnly)
		{
			if (CastField<FStructProperty>(*Property) != NULL)
			{
				continue;
			}

			FObjectPropertyBase *pObjectProperty = CastField<FObjectPropertyBase>(*Property);
			if (pObjectProperty && !pObjectProperty->PropertyClass->IsChildOf(UDataAsset::StaticClass()))
			{
				continue;
			}
		}

		//Special case for Variable[X] type of arrays
		if (Property->ArrayDim != 1)
		{
			//Check don't save object properties
			if (!InFile && !CheckSaveObjectProperty(*Property, NULL))
				continue;

			FArrayData NewArray;
			NewArray.Data.SetNum(0);

			for (int32 Index = 0; Index < Property->ArrayDim; Index++)
			{
				void *ValuePtr = Property->ContainerPtrToValuePtr<void>(InObject, Index);

				//Handle saving 
				FString SaveString;
				if (!HandleSaveObject(InFile, *Property, InObject, ValuePtr, 0, SaveString) && !HandleSaveStruct(InFile, *Property, InObject, ValuePtr, SaveString))
				{
					Property->ExportTextItem_Direct(SaveString, ValuePtr, ValuePtr, InObject, TextPortFlags);
				}

				NewArray.Data.Add(SaveString);
			}

			Arrays.Emplace(Property->GetFName(), NewArray);
		}
		//Regular gosh darn variable
		else
		{
			void *ValuePtr = Property->ContainerPtrToValuePtr<void>(InObject);

			//Check don't save object properties
			if (!InFile && !CheckSaveObjectProperty(*Property, ValuePtr))
			{
				UE_LOG(LogTemp, Warning, TEXT("Cannot save property %s in object %s"), *Property->GetName(), *InObject->GetName());
				continue;
			}

			//Handle saving 
			FString SaveString;
			if (HandleSaveObject(InFile, *Property, InObject, ValuePtr, 0, SaveString))
			{
#if WITH_EDITOR
				if (bDebug)
				{
					UE_LOG(LogTemp, Warning, TEXT("HandleSaveObject property \"%s\" in object \"%s\" with value \"%s\""), 
					*Property->GetName(), 
					*InObject->GetName(),
					*SaveString);
				}
#endif 
			}
			else if (HandleSaveStruct(InFile, *Property, InObject, ValuePtr, SaveString))
			{
#if WITH_EDITOR
				if (bDebug)
				{
					UE_LOG(LogTemp, Warning, TEXT("HandleSaveStruct property \"%s\" with value \"%s\""), *Property->GetName(), *SaveString);
				}
#endif 
			}
			else
			{
				Property->ExportTextItem_Direct(SaveString, ValuePtr, ValuePtr, InObject, TextPortFlags);

#if WITH_EDITOR
				class FObjectPropertyBase *pObjectProperty = bDebug ? CastField<FObjectPropertyBase>(*Property) : NULL;
				if (pObjectProperty)
				{
					UE_LOG(LogTemp, Warning, TEXT("Object property \"%s\" with value \"%s\""), *pObjectProperty->GetName(), *SaveString);
				}
#endif //
			}

			Singles.Emplace(Property->GetFName(), SaveString);
		}
	}

	//
	if (InFile)
	{
		class ISaveInterface *pInterface = Cast<ISaveInterface>(InObject);
		if (pInterface)
		{
			TArray<class UObject*> Assets;
			pInterface->GetCustomAssetsToLoad(Assets);
			for (int32 i=0; i<Assets.Num(); i++)
			{
				if (IsValid(Assets.GetData()[i]) && ShouldSaveIntoAssetArray(Assets.GetData()[i]))
				{
					InFile->AssetsToLoad.AddUnique(Assets.GetData()[i]);
				}
			}
		}
	}

#if WITH_EDITOR
	if (bDebug)
	{
		for (auto It = Singles.CreateConstIterator(); It; ++It)
		{
			UE_LOG(LogTemp, Error, TEXT("Property \"%s\" value \"%s\""), *It.Key().ToString(), *It.Value());
		}
	}
#endif 
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::SaveCustomData(class UObject *InObject, FCustomSaveData &InData)
{
	if (!IsValid(InObject))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Null object on SaveCustomData!"));
		return false;
	}

	InData.Class = InObject->GetClass();
	InData.Name = InObject->GetFName();
	InData.OuterObjectIndex = INDEX_NONE;

	if (InData.ObjectIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Fatal, TEXT("Invalid object index!"));
	}

	ISaveInterface *pInterface = Cast<ISaveInterface>(InObject);
    if (pInterface)
    {
		pInterface->Runtime_PreSave();

		InData.Recreate = pInterface->ShouldDeleteOnRestore();
	}
	else
	{
		InData.Recreate = false;
	}

	SaveCustomData_Internal(this, InObject, InData.Singles, InData.Arrays, InData.Maps);

	if (pInterface)
    {
		pInterface->Runtime_PostSave();
	}

	return InData.GetCount() > 0;
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::SaveSimpleProperties(class UObject *InObject, FSimpleSaveData &InData, bool InIgnoreNativeProperties)
{
	InData.Singles.Reset();
	InData.Arrays.Reset();
	InData.Maps.Reset();

	SaveCustomData_Internal(NULL, InObject, InData.Singles, InData.Arrays, InData.Maps, true, NULL, InIgnoreNativeProperties);
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::LoadSimpleProperties(class UObject *InObject, const FSimpleSaveData &InData)
{
	RestoreCustomData_Internal(NULL, InObject, InData.Singles, InData.Arrays, InData.Maps);
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::DebugSavedProperties(const FSimpleSaveData &InData)
{
	for (auto It = InData.Singles.CreateConstIterator(); It; ++It)
	{
		UE_LOG(LogTemp, Error, TEXT("Saved single property \"%s\" with value \"%s\""), *It.Key().ToString(), *It.Value());
	}

	for (auto It = InData.Arrays.CreateConstIterator(); It; ++It)
	{
		UE_LOG(LogTemp, Error, TEXT("Saved array property \"%s\""), *It.Key().ToString());
	}

	for (auto It = InData.Maps.CreateConstIterator(); It; ++It)
	{
		UE_LOG(LogTemp, Error, TEXT("Saved map property \"%s\""), *It.Key().ToString());
	}
}

//=================================================================
// 
//=================================================================
bool USimpleSaveFile::LoadGame(const class UObject *WorldContext, FString Filename)
{
	if (!IsValid(WorldContext))
	{
		UE_LOG(LogTemp, Error, TEXT("No world context!"));
		return false;
	}

	//
	class USaveGameInstance *pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!pGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get game instance!"));
		return false;
	}

	//
	class USimpleSaveFile *pLoadGame = Cast<USimpleSaveFile>(UGameplayStatics::LoadGameFromSlot(Filename, 0));
	if (!pLoadGame)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load game \"%s\""), *Filename);
		return false;
	}

	//If no map name defined in the level then oh no
	if (pLoadGame->CurrentMapName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("Save game has no map name!"));
		return false;
	}

	//If no map name defined in the level then oh no
	if (pLoadGame->CurrentMapName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("Save game has no map name!"));
		return false;
	}

	//
	pGameInstance->OnNewGame(const_cast<UObject*>(WorldContext), true);

	//Mark us as loading the game
	pGameInstance->StartLoading(pLoadGame);

	FSimpleHeaderData Header;
	USimpleSaveHeader::GetHeaderDataFor(WorldContext, Filename, Header);

	pGameInstance->SetLoadingScreenIconsFromLevelChangeTags(Header.LevelChangeTags);

	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
	LoadingScreenModule.StartInGameLoadingScreen(true, 1.0f);

	//GEngine->ForceGarbageCollection(true);
	UGameplayStatics::OpenLevel(WorldContext, pLoadGame->CurrentMapName);
	return true;
}

//=================================================================
// 
//=================================================================
bool GatherReferencesToAssetsOfClassFromProperty(class UObject *InObject, class FProperty* InProperty, void *InRawData, TSubclassOf<class UObject> InClass, TArray<class UObject*>& OutObjects)
{
	class FStructProperty *StructProperty = CastField<FStructProperty>(InProperty);
	if (StructProperty)
	{
		bool bSuccess = false;

		//
		for (TFieldIterator<FProperty> Property(StructProperty->Struct); Property; ++Property)
		{
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(InRawData);

			if (GatherReferencesToAssetsOfClassFromProperty(InObject, *Property, ValuePtr, InClass, OutObjects))
			{
				bSuccess = true;
			}
		}

		return bSuccess;
	}

	class FArrayProperty* ArrayProperty = CastField<FArrayProperty>(InProperty);
	if (ArrayProperty)
	{
		bool bSuccess = false;

		/*
		FScriptArrayHelper_InContainer ArrayHelper(ArrayProperty, InRawData);
		for (int32 i = 0; i < ArrayHelper.Num(); i++)
		{
			if (GatherReferencesToAssetsOfClassFromProperty(InObject, ArrayProperty->Inner, ArrayHelper.GetRawPtr(i), InClass, OutObjects))
			{
				bSuccess = true;
			}
		}
		*/

		return bSuccess;
	}

	//Check that object property
	class FObjectProperty * ObjectProperty = CastField<FObjectProperty>(InProperty); //CastToObjectProperty(InProperty);
	if (!ObjectProperty)
	{
		return false;
	}

	/*
	if (CastField<FSoftObjectProperty>(InProperty) != NULL)
	{
		return false;
	}
	*/

	class UObject* pObject = ObjectProperty->GetObjectPropertyValue(InRawData);
	if (!IsValid(pObject))
	{
		return false;
	}

	if (!pObject->GetClass()->IsChildOf(InClass))
	{
		return false;
	}

	if (OutObjects.Contains(pObject))
	{
		return false;
	}

	//UE_LOG(LogTemp, Error, TEXT("Adding object %s from %s of class %s"), *pObject->GetName(), *InObject->GetName(), *InObject->GetClass()->GetName());
	OutObjects.Add(pObject);
	return true;
}

//=================================================================
// 
//=================================================================
void GatherReferencedAssetsOfClassFromObject(class UObject* InObject, TSubclassOf<class UObject> InClass, TArray<class UObject*>& OutObjects, bool InSaveGameOnly)
{
	int32 TextPortFlags = PPF_SimpleObjectText;

	//Go through all the properties
	for (TFieldIterator<FProperty> Property(InObject->GetClass()); Property; ++Property)
	{
		//
		if (InSaveGameOnly && (Property->GetPropertyFlags() & CPF_SaveGame) == 0)
			continue;

		//Handle arrays separately
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(*Property);
		if (ArrayProperty)
		{
			FScriptArrayHelper_InContainer ArrayHelper(ArrayProperty, InObject);
			for (int32 i = 0; i < ArrayHelper.Num(); i++)
			{
				GatherReferencesToAssetsOfClassFromProperty(InObject, ArrayProperty->Inner, ArrayHelper.GetRawPtr(i), InClass, OutObjects);
			}

			continue;
		}

		//Handle maps separately
		FMapProperty* MapProperty = CastField<FMapProperty>(*Property);
		if (MapProperty)
		{
			FScriptMapHelper_InContainer MapHelper(MapProperty, InObject, 0);

			for (int32 SparseElementIndex = 0; SparseElementIndex < MapHelper.GetMaxIndex(); ++SparseElementIndex)
			{
				if (!MapHelper.IsValidIndex(SparseElementIndex))
					continue;
					
				GatherReferencesToAssetsOfClassFromProperty(InObject, MapHelper.GetKeyProperty(), MapHelper.GetKeyPtr(SparseElementIndex), InClass, OutObjects);
				GatherReferencesToAssetsOfClassFromProperty(InObject, MapHelper.GetValueProperty(), MapHelper.GetValuePtr(SparseElementIndex), InClass, OutObjects);
			}

			continue;
		}

		//Special case for Variable[X] type of arrays
		if (Property->ArrayDim != 1)
		{
			for (int32 Index = 0; Index < Property->ArrayDim; Index++)
			{
				void* ValuePtr = Property->ContainerPtrToValuePtr<void>(InObject, Index);

				GatherReferencesToAssetsOfClassFromProperty(InObject, *Property, ValuePtr, InClass, OutObjects);
			}
		}
		//Regular gosh darn variable
		else
		{
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(InObject);	

			GatherReferencesToAssetsOfClassFromProperty(InObject, *Property, ValuePtr, InClass, OutObjects);
		}
	}
}

//=================================================================
// 
//=================================================================
void USimpleSaveFile::GatherReferencedAssetsOfClass_Internal(class UObject *InWorldContext, TSubclassOf<class UObject> InClass, TArray<class UObject*> &OutObjects, TArray<TSubclassOf<class UObject>> InIgnoredActorsAndComponents, bool InSaveGameOnly)
{
	for (TActorIterator<AActor> ActorItr(InWorldContext->GetWorld()); ActorItr; ++ActorItr)
	{
		// Follow iterator object to my actual actor pointer
		AActor* MyActor = *ActorItr;

		if (InIgnoredActorsAndComponents.Contains(MyActor->GetClass()))
			continue;

		GatherReferencedAssetsOfClassFromObject(MyActor, InClass, OutObjects, InSaveGameOnly);

		//
		TArray<class UActorComponent*> MyComponents;
		MyActor->GetComponents(MyComponents);

		for (int32 i=0; i<MyComponents.Num(); i++)
		{
			if (InIgnoredActorsAndComponents.Contains(MyComponents.GetData()[i]->GetClass()))
				continue;

			GatherReferencedAssetsOfClassFromObject(MyComponents.GetData()[i], InClass, OutObjects, InSaveGameOnly);
		}
	}

}