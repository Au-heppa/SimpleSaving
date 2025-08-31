// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveData.h"
#include "SimpleSaveFile.generated.h"

//=================================================================
// 
//=================================================================
UCLASS()
class SIMPLESAVING_API USimpleSaveFile : public USaveGame
{
	GENERATED_BODY()
	
	//=================================================================
	// 
	//=================================================================
public:

	//
	static const FName Name_DontSave;
	static const FName Name_ForceSave;
	static const FName Name_GameInstance;
	static const FName Name_PlayerController;
	static const FName Name_PlayerPawn;
	static const FName Name_GameState;

	//=================================================================
	// 
	//=================================================================
public:

	//
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static bool AutoSave(const class UObject *WorldContextObject, bool InMultiLevel = true);

	//
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static bool SaveGame(const class UObject *WorldContextObject, FString Filename, bool InMultiLevel = true, bool InChangeLevel = false);

	//
	UFUNCTION(BlueprintCallable, meta=(CallableWithoutWorldContext))
	static FString ParseSaveFilename(FString InFilename);

	//
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static bool LoadGame(const class UObject *WorldContextObject, FString Filename);
	
	//
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static bool CopySaveFile(const class UObject *WorldContextObject, FString Source, FString Destination);

	//
	UFUNCTION(BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static bool CanSave(const class UObject *WorldContextObject);

	//Saves only integers, booleans, floats, strings, texts
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static void SaveSimpleProperties(class UObject *InObject, FSimpleSaveData &InData, bool InIgnoreNativeProperties);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static void LoadSimpleProperties(class UObject *InObject, const FSimpleSaveData &InData);

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject"))
	static void DebugSavedProperties(const FSimpleSaveData &InData);

	//Gather classes we might want to load
	bool GatherClassesToLoad(class UObject *WorldContext, TArray<class TSoftClassPtr<class UObject>> &OutClasses);

#if WITH_EDITOR
	//
	bool DebugSaveGame(const class UObject* WorldContext, bool InMultiLevel);
#endif //

	//=================================================================
	// 
	//=================================================================
private:

	//Gather list of assets that are referenced by actors or their components.
	static void GatherReferencedAssetsOfClass_Internal(class UObject* InWorldContext, TSubclassOf<class UObject> InClass, TArray<class UObject*>& OutObjects, TArray<TSubclassOf<class UObject>> InIgnoredActorsAndComponents, bool InSaveGameOnly = false);

public:

	//
	template<class T>
	static void GatherReferencedAssetsOfClass(class UObject* InWorldContext, TArray<T*>& OutAssets, TArray<TSubclassOf<class UObject>> InIgnoredActorsAndComponents, bool InSaveGameOnly = false)
	{
		TArray<class UObject*> Objects;
		GatherReferencedAssetsOfClass_Internal(InWorldContext, T::StaticClass(), Objects, InIgnoredActorsAndComponents, InSaveGameOnly);
		OutAssets.SetNum(Objects.Num());
		for (int32 i=0; i<Objects.Num(); i++)
		{
			OutAssets.GetData()[i] = (T*)Objects.GetData()[i];
		}
	}

private:

	UFUNCTION(BlueprintCallable)
	bool FindFirstReferenceTo(FString InString, FString &OutObjectThatReferences);

	//
	bool SaveData(const class UObject *WorldContext, bool InMultiLevel, float InTime, bool InCleapUp = true);

	//
	FLevelSaveData *SaveLevelData(class USaveGameInstance *InInstance, class APlayerController *InController, class APawn *InPawn, const class UObject *WorldContext);

	//
	FCustomSaveData *AddObjectToSave(FLevelSaveData *InLevelData, class UObject *InObject, const FName &InTag);

	//
	FCustomSaveData *FindObjectToSave(FLevelSaveData *InLevelData, class UObject *InObject, bool &OutGlobal);

	//
	class UObject* GetObjectByTag(const FName& InTag, int32 InIndex, bool InGlobal) const;

	//
	bool RestoreObjectByTag(class UObject *InObject, const FName &InTag, bool InGlobal);

	//
	bool RestoreActorByTag(class AActor *InActor, const FName &InTag, bool InGlobal);

	//
	bool GetObjectTag(class UObject* InObject, FName& OutTag, int32& OutIndex, bool& OutGlobal);

	//
	FActorSaveData *AddActorToSave(FLevelSaveData *InLevelData, class AActor *InActor, const FName &InTag);

	void AddComponentToSave(FLevelSaveData* InLevelData, const FName& InTag, FActorSaveData* InActorData, class UActorComponent* InComponent);

	//
	void GatherAttachParents(TArray<class USceneComponent*>& AttachParents);

	//
	bool SaveActor(class AActor *InActor, FActorSaveData &InData);

	//
	bool SaveComponent(class UActorComponent *InComponent, FComponentSaveData &InData);

	//
	bool SaveCustomData(class UObject *InObject, FCustomSaveData &InData);

	//
	void SaveObjectOuters(FCustomSaveData &InData, bool InGlobal);

	//
	void SaveActorAttachParents(FActorSaveData &InData, bool InGlobal);

public:

	//
	void RestoreActor(FLevelSaveData *InLevelData, class AActor *InActor, const FActorSaveData &InData);

	//
	void RestoreComponent(class AActor *InActor, class UActorComponent *InComponent, const FComponentSaveData &InData);

	//
	void RestoreCustomData(class UObject *InObject, const FCustomSaveData &InData);

private:

	static void RestoreCustomData_Internal(class USimpleSaveFile *InFile, class UObject *InObject, const TMap<FName, FString> &Singles, const TMap<FName, FArrayData> &Arrays, const TMap<FName, FMapData> &Maps);

public:

	//
	static bool GetRequiredPointers(class UObject* WorldContext, class USaveGameInstance*& OutGameInstance, class APlayerController*& OutController, class APawn*& OutPlayer, class AGameStateBase*& OutGameState);
	bool HandleRestore_BasicObjects(class UObject* WorldContext, float& OutTimeSkip, TMap<FName, class AActor*> &CustomTags);
	bool HandleRestore_ClearObjects(class UObject* WorldContext);
	bool HandleRestore_RecreateAllObjects(class UObject* WorldContext, TMap<FName, class AActor*>& CustomTags);
	bool HandleRestore_RestoreActors(class UObject* WorldContext, int32 &Start, int32 MaxCount);
	bool HandleRestore_RestoreDynamicObjects(class UObject* WorldContext, int32& Start, int32 MaxCount);
	bool HandleRestore_CalculateLevelChangeActor(class UObject* WorldContext);
	bool HandleRestore_CallOnRestore(class UObject* WorldContext);
	bool HandleRestore_Finish(class UObject* WorldContext, bool InTriggerPostLevelChange);

public:

	//
	bool HandleRestore(class UObject* WorldContext, float &OutTimeSkip, bool InTriggerPostLevelChange);

	//
	static bool TriggerPostLevelChange(class UObject *WorldContext);

	//
	class UObject *OnRestoreObject(const FCustomSaveData &InData, class UObject* InObject, bool InGlobal);

	//
	void OnRestoreActor(const FActorSaveData &InData, class AActor *InActor, bool InGlobal);

	//
	class UActorComponent *GetComponent(class AActor *InActor, const FComponentSaveData &InData) const;

	//
	class UObject *GetRestoreObject(const FCustomSaveData &InData, bool InGlobal) const;

	//
	class UObject* GetRestoreObjectIndex(int32 ObjectIndex, bool InGlobal) const;

	//
	void ClearActorsOnRestore(class UObject *WorldContext, bool IsLevelChange);

	//
	class UObject* GetObjectOuter(FLevelSaveData* InLevelData, const FCustomSaveData& InData) const;

	//
	const FCustomSaveData* GetOuterObjectData(FLevelSaveData* InLevelData, const FCustomSaveData& InData) const;

	//
	void FinishLoading();

	//
	void ClearCurrentLevelAndGlobalData(class UObject* WorldContextObject);

	//
	bool ClearLevelData(const FName &InLevel);

	//
	static bool StripLevelNameString(const FString& InString, FString& OutString);

	//
	void SetCurrentMapName(const class UObject * const InObject);

	//
	FORCEINLINE const FName &GetCurrentLevelName() const { return CurrentMapName; }

	//=================================================================
	// SPECIAL CASE HANDLING SAVE
	//=================================================================
public:

	//
	static bool IsValidDynamicObject(class UObject* InObject);

private:

	//
	static class FObjectPropertyBase *CastToObjectProperty(class FProperty* InProperty);

	//
	static bool ForceSaveAsSoftObject(class UObject *InObject);

	//
	FString GetSavingObjectPrefix(bool InGlobal) const;

public:

	//
	UFUNCTION(BlueprintCallable)
	FString GetSavingObjectString(bool InGlobal, const FName& InTag, int32 InIndex) const;

	UFUNCTION(BlueprintCallable, meta=(CallableWithoutWorldContext=true))
	static FString GetSizeStringForCustomData(const FCustomSaveData &InData) { return InData.GetSizeString(); }

	//
	UFUNCTION(BlueprintPure)
	FString GetLevelSizeString(int32 InLevel) const;

	//
	UFUNCTION(BlueprintPure)
	FString GetGlobalSizeString() const;

	//
	UFUNCTION(BlueprintPure)
	FString GetTotalSizeString() const;

private:

	/*
	//
	UFUNCTION(BlueprintPure)
	FString GetSavedObjectString(const FCustomSaveData &InData);
	*/

	//
	static const FString &GetSoftObjectPrefix();

	//
	static bool GetSoftObjectString(class UObject *InObject, FString &OutString);

	//
	static bool HandleSaveStruct(class USimpleSaveFile *InFile, class FProperty *InProperty, class UObject* InObject, void* InRawData, FString& OutString);

	//
	bool HandleSaveObject_Internal(class FProperty* InProperty, class UObject* InObject, void* InRawData, int32 InIndex, FString& OutString);

	//
	FORCEINLINE static bool HandleSaveObject(class USimpleSaveFile *InFile, class FProperty* InProperty, class UObject* InObject, void* InRawData, int32 InIndex, FString& OutString)
	{
		return InFile != NULL && InFile->HandleSaveObject_Internal(InProperty, InObject, InRawData, InIndex, OutString);
	}

private:

	//
	static void SaveCustomData_Internal(class USimpleSaveFile *InFile, class UObject *InObject, TMap<FName, FString> &Singles, TMap<FName, FArrayData> &Arrays, TMap<FName, FMapData> &Maps, bool SimplePropertiesOnly = false, TArray<FName> *IgnoredProperties = NULL, bool InIgnoreNativeProperties = false);
	
	//=================================================================
	// SPECIAL CASE HANDLING RESTORE
	//=================================================================
private:	
	
	//
	void RecreateDynamicObjects(FLevelSaveData *InLevelData, const TArray<FCustomSaveData> &InData, bool InGlobal);

	//
	class UObject *RecreateDynamicObject(FLevelSaveData *InLevelData, const FCustomSaveData &InData, bool InGlobal);

	//
	void RespawnOrFindActors(class UObject *WorldContext, const TArray<FActorSaveData> &InData, TArray<class UObject*> &InObjects, bool InGlobal);

	//
	static bool HandleRestoreStruct(class USimpleSaveFile *InFile, class FProperty *InProperty, const FString &InValue, void *InRawData, int32 InIndex, class UObject *InObject);

	//
	static bool HandleFixSoftObject(class UObject *InObject, class FProperty *InProperty, FString &InValue);

	//
	bool HandleRestoreObject_Internal(class FProperty* InProperty, const FString& InValue, void* InRawData, int32 InIndex, class UObject *InObject);

	//
	FORCEINLINE static bool HandleRestoreObject(class USimpleSaveFile *InFile, FProperty* InProperty, const FString& InValue, void* InRawData, int32 InIndex, class UObject *InObject)
	{
		return InFile != NULL && InFile->HandleRestoreObject_Internal(InProperty, InValue, InRawData, InIndex, InObject);
	}

	//=================================================================
	// 
	//=================================================================
public:

	//
	FORCEINLINE const TArray<FActorSaveData> &GetGlobalActors() const { return GlobalActors; }

	//
	FORCEINLINE const TArray<FCustomSaveData> &GetCustomObjects() const { return CustomObjects; }

	//
	const FLevelSaveData *GetLevelSaveData(const FName &InLevel) const;

	//
	FORCEINLINE const TArray<TSoftObjectPtr<class UObject>> &GetAssetsToLoad() const { return AssetsToLoad; }

private:

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Data", meta=(AllowPrivateAccess=true))
	TArray<FActorSaveData> GlobalActors;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Data", meta=(AllowPrivateAccess=true))
	FName CurrentMapName;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Data", meta=(AllowPrivateAccess=true))
	bool MultiLevelSaveGame;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Data", meta=(AllowPrivateAccess=true))
	TArray<FLevelSaveData> Levels;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Data", meta=(AllowPrivateAccess=true))
	TArray<FCustomSaveData> CustomObjects;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Data", meta=(AllowPrivateAccess=true))
	float SaveTime;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Data", meta=(AllowPrivateAccess=true))
	TArray<TSoftObjectPtr<class UObject>> AssetsToLoad;

	//=================================================================
	// 
	//=================================================================
public:

	//
	FORCEINLINE const TArray<class UObject*> &GetGlobalSaveObjects() const { return GlobalSaveObjects; }

	//
	FORCEINLINE const TArray<class UObject*> &GetLocalSaveObjects() const { return LocalSaveObjects; } 

	//
	static FString GatherObjectCrashData(class UObject *InObject);

private:

	//
	FLevelSaveData *CurrentLevelData;

	//
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Runtime", meta=(AllowPrivateAccess=true))
	TArray<class UObject*> GlobalSaveObjects;

	//
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Runtime", meta=(AllowPrivateAccess=true))
	TArray<class UObject*> LocalSaveObjects;


	//
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Runtime", meta=(AllowPrivateAccess=true))
	TArray<class UObject*> KeepInMemory;

	//Array of classes we gathered so we don't need to do it constantly.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="Runtime", meta=(AllowPrivateAccess=true))
	TArray<TSoftClassPtr<class UObject>> GatheredClasses;
};

//=================================================================
// 
//=================================================================
FORCEINLINE const FLevelSaveData *USimpleSaveFile::GetLevelSaveData(const FName &InLevel) const
{
	for (int32 i=Levels.Num()-1; i>=0; i--)
	{
		if (Levels.GetData()[i].LevelName == InLevel)
		{
			return &Levels.GetData()[i];
		}
	}

	return NULL;
}

//=================================================================
// 
//=================================================================
FORCEINLINE int32 FindActorDataIndex(int32 ObjectIndex, const TArray<FActorSaveData> &InArray)
{
	for (int32 j=0; j<InArray.Num(); j++)
	{
		if (InArray.GetData()[j].Custom.ObjectIndex == ObjectIndex)
			return j;
	}

	return INDEX_NONE;
}

//=================================================================
// 
//=================================================================
FORCEINLINE FActorSaveData *FindActorData(int32 ObjectIndex, TArray<FActorSaveData> &InArray)
{
	for (int32 j=0; j<InArray.Num(); j++)
	{
		if (InArray.GetData()[j].Custom.ObjectIndex == ObjectIndex)
			return &InArray.GetData()[j];
	}

	return NULL;
}

//=================================================================
// 
//=================================================================
FORCEINLINE int32 FindCustomDataIndex(int32 ObjectIndex, const TArray<FCustomSaveData> &InArray)
{
	for (int32 j=0; j<InArray.Num(); j++)
	{
		if (InArray.GetData()[j].ObjectIndex == ObjectIndex)
			return j;
	}

	return INDEX_NONE;
}

//=================================================================
// 
//=================================================================
FORCEINLINE FCustomSaveData *FindCustomData(int32 ObjectIndex, TArray<FCustomSaveData> &InArray)
{
	for (int32 j=0; j<InArray.Num(); j++)
	{
		if (InArray.GetData()[j].ObjectIndex == ObjectIndex)
			return &InArray.GetData()[j];
	}

	return NULL;
}

//=================================================================
// 
//=================================================================
FORCEINLINE void USimpleSaveFile::RestoreCustomData(class UObject* InObject, const FCustomSaveData& InData)
{
	RestoreCustomData_Internal(this, InObject, InData.Singles, InData.Arrays, InData.Maps);
}