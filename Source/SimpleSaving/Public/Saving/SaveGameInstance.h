// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SaveFileList.h"
#include "GameplayTagContainer.h"
#include "SimpleHeaderData.h"
#include "SaveGameInstance.generated.h"

//=================================================================
// 
//=================================================================
UCLASS()
class SIMPLESAVING_API USaveGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	USaveGameInstance(const FObjectInitializer& ObjectInitializer);
	
	//=================================================================
	// NEW GAME
	//=================================================================

public:

	//Start a new game
	UFUNCTION(BlueprintCallable)
	bool StartNewGame(class UObject *WorldContextObject, TSoftObjectPtr<class UWorld> InLevel, FGameplayTagContainer InLevelChangeTags);

	//
	UFUNCTION(BlueprintNativeEvent)
	void OnNewGame(class UObject* WorldContextObject, bool LoadingFromSave);
	virtual void OnNewGame_Implementation(class UObject* WorldContextObject, bool LoadingFromSave) { }

	//
	virtual bool GatherHeaderData(const class UObject* WorldContextObject, FSimpleHeaderData &OutData, bool& OutRequestScreenshot) const { return false; }

	//
	virtual bool GatherLevelChangeTags(const class UObject* WorldContextObject, FGameplayTagContainer &OutTags) const { return false; }

	//
	void SetLoadingScreenIconsFromLevelChangeTags(const FGameplayTagContainer& InContainer);

	//
	UFUNCTION(BlueprintPure, meta = (CallableWithoutWorldContext))
	static FText GetLoadingScreenText();

	//=================================================================
	// SAVING - FUNCTIONS
	//=================================================================

public:

	//
	virtual void GetGlobalActorTags(class APlayerController *InController, class APawn *InPawn, bool InRestore, TMap<FName, class AActor*> &OutActors) { }

	//
	virtual void GetLocalActorTags(class APlayerController *InController, class APawn *InPawn, TMap<FName, class AActor*> &OutActors) { }

	//
	void StartLoading(class USimpleSaveFile *InLoadGame);

	//
	void SetSaveGame(class USimpleSaveFile *InSaveGame);

	//
	virtual bool PreChangeLevel(class UObject* WorldContextObject, const TSoftObjectPtr<class UWorld>& InLevel, FGameplayTag InPositionTag);

	//
	FORCEINLINE bool IsLoading() const { return LoadGame != NULL && bIsLoading; }
	FORCEINLINE class USimpleSaveFile *GetLoadGame() const { return LoadGame; }

	//
	void FinishLoading(class UObject *WorldContextObject);

#if WITH_EDITOR
	//
	bool CheckSuccessLoad(class UObject *WorldContextObject, bool InMultiLevel);
#endif //

	//
	UFUNCTION(BlueprintCallable)
	bool HandleRestore(class UObject *WorldContextObject, bool StopLoadingScreen, float &OutTimeSkip, bool TriggerPostLevelChange);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="CloseLoadingScreen"))
	void DoStopLoadingScreen();

	//=================================================================
	// SAVING - VARIABLES
	//=================================================================

private:

	//
	UPROPERTY(VisibleAnywhere, Category="Saving", BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class USimpleSaveFile *LoadGame = NULL;

	//
	UPROPERTY(VisibleAnywhere, Category="Saving", BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	bool bIsLoading;

#if WITH_EDITORONLY_DATA
	//
	UPROPERTY()
	class USimpleSaveFile *DebugSaveFile = NULL;

	//
	UPROPERTY(EditAnywhere, Category="Saving")
	bool DebugSaving = false;
#endif //

	//=================================================================
	// LEVEL CHANGE - FUNCTIONS
	//=================================================================
public:

	UFUNCTION(BlueprintCallable)
	bool ChangeLevel(class UObject *WorldContextObject, TSoftObjectPtr<class UWorld> InLevel, FGameplayTag InPositionTag);

	//
	FORCEINLINE bool InLevelChange() const { return bInLevelChange; }

	//
	FORCEINLINE const FGameplayTag &GetLevelChangePositionTag() const { return LevelChangePositionTag; }

	//=================================================================
	// LEVEL CHANGE - VARIABLES
	//=================================================================
private:

	UPROPERTY(EditAnywhere, Category = "Level Change", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	bool UseRestoreHandler = true;

	UPROPERTY(EditAnywhere, Category="Level Change", BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	FString LevelChangeFilename;

	//
	UPROPERTY(VisibleAnywhere, Category="Level Change", BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	bool bInLevelChange;

	//
	UPROPERTY(VisibleAnywhere, Category="Level Change", BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	FGameplayTag LevelChangePositionTag;

	//
	UPROPERTY(EditAnywhere, Category = "Level Change", BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TMap<FGameplayTag, class UMaterialInterface*> LevelChangeIcons;

public:

	UPROPERTY(VisibleAnywhere, Category="Level Change", BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TWeakObjectPtr<class AActor> LevelChangeActor = NULL;

	//=================================================================
	// SAVE FILE LIST
	//=================================================================
public:

	//
	UFUNCTION(BlueprintCallable)
	void DestroySaveFileList();

	//
	UFUNCTION(BlueprintCallable)
	bool GenerateSaveFileList();

	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly = true))
	static bool GenerateSaveFilesListEditor(TArray<FString> &OutFiles);

	//
	bool UpdateSaveFile(const FString &InFilename);

	//
	void SortSaveFiles();

	//
	FORCEINLINE const TArray<FSaveFileList> &GetSaveFiles() const { return SaveFiles; }

private:

	//
	UPROPERTY(VisibleAnywhere, Category="Save Files", BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TArray<FSaveFileList> SaveFiles;

	//
	UPROPERTY(VisibleAnywhere, Category="Save Files", BlueprintReadWrite, meta=(AllowPrivateAccess=true))
	FString SaveFilename;

	//=================================================================
	// TIME
	//=================================================================
public:

	//
	UFUNCTION(BlueprintPure)
	float GetTotalTime(const class UObject * const WorldContextObject) const;

	UFUNCTION(BlueprintPure, Category="Utilities|Time", meta=(WorldContext="WorldContextObject"))
	static float GetTotalTimeSeconds(const UObject* WorldContextObject);

	//
	UPROPERTY()
	float TotalTime;

	//=================================================================
	// HEADER DATA
	//=================================================================
public:

	//
	UPROPERTY(Transient)
	class USimpleSaveHeader *SaveHeaderData = NULL;

	//==============================================================================================================
	// DELEGATES
	//==============================================================================================================
public:

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSaveGameInstanceEvent);

	//
	UPROPERTY(BlueprintAssignable)
	FSaveGameInstanceEvent OnRestoreFinished;
};
