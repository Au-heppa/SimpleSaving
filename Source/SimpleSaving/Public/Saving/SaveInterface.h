// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "UObject/Interface.h"
#include "SaveInterface.generated.h"

//=================================================================
// 
//=================================================================
UINTERFACE(BlueprintType)
class SIMPLESAVING_API USaveInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
		
};

//=================================================================
// 
//=================================================================
class SIMPLESAVING_API ISaveInterface
{
	GENERATED_IINTERFACE_BODY()

	//=================================================================
	// 
	//=================================================================
public:

	//
	virtual FName GetSavingTag() const { return NAME_None; }

	//
	virtual bool BlockSaving() const { return false; }

#if WITH_EDITOR
	virtual FString GetBlockSavingReason() const { return TEXT("None"); }
#endif

	//
	virtual bool ShouldDeleteOnRestore() const { return false; }

	//
	virtual bool ShouldRespawnOnLevelChange() const { return false; }

	//
	virtual bool ShouldSave() const { return true; }

	//
	virtual void GetCustomAssetsToLoad(TArray<class UObject*> &OutAssets) { }

	//
	virtual void Runtime_PreSave() { }

	//
	virtual void Runtime_PostSave() { }

	//
	virtual void PreRestore() { }

	//
	virtual void OnRestore(class USaveGameInstance *InGameInstance, class APlayerController *InController) { }

	//
	virtual void HandleReattach(class AActor *InActor, class USceneComponent *InComponent, const FName &InSocket, const FTransform &InRelativeTransform);

	//
	virtual FName GetAttachedTagName(class AActor *InActor) const { return NAME_None; }

	//
	FName EnsureUniqueSavingTag(class AActor *InActor, const FName &InSavingTag, const TCHAR *InDefaultName);
};