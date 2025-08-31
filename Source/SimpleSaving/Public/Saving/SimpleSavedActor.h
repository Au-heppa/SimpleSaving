// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveInterface.h"
#include "SimpleSavedActor.generated.h"

//=================================================================================================
// Simple actor class allowing easily to change the saving settings
//=================================================================================================
UCLASS(Blueprintable)
class SIMPLESAVING_API ASimpleSavedActor : public AActor, public ISaveInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASimpleSavedActor();

	virtual void OnRestore(class USaveGameInstance *InGameInstance, class APlayerController *InController) override;

	//
	UFUNCTION(BlueprintNativeEvent)
	void OnRestored();
	virtual void OnRestored_Implementation() { }

	//
	virtual bool ShouldDeleteOnRestore() const override { return bShouldDeleteOnRestore; }

	//
	virtual bool ShouldSave() const override { return bShouldSave; }

	//
	virtual bool BlockSaving() const override { return bBlockSaving; }

	//
	virtual bool ShouldRespawnOnLevelChange() const { return bShouldRespawnOnLevelChange; }

public:

	//Should this actor save
	UPROPERTY(Category="Saving", EditAnywhere, BlueprintReadWrite)
	bool bShouldSave = true;

	//Should this actor be completely recreated
	UPROPERTY(Category="Saving", EditAnywhere, BlueprintReadWrite)
	bool bShouldDeleteOnRestore = false;

	//Should this actor be completely recreated
	UPROPERTY(Category = "Saving", EditAnywhere, BlueprintReadOnly)
	bool bShouldRespawnOnLevelChange = false;

	//Prevent the game from being saved
	UPROPERTY(Category="Saving", EditAnywhere, BlueprintReadWrite)
	bool bBlockSaving = false;
};
