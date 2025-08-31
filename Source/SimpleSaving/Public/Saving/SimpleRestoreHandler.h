// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimpleRestoreHandler.generated.h"

//=================================================================
// 
//=================================================================
UCLASS()
class SIMPLESAVING_API ASimpleRestoreHandler : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASimpleRestoreHandler();

	static void CreateRestoreHandler(class UObject* WorldContext, class USaveGameInstance* InGameInstance, class USimpleSaveFile* InFile, float& OutTimeSkip, bool InTriggerPostLevelChange);

public:
	// Called every frame
	void Tick(float DeltaTime) override;

	void Initialize(class USaveGameInstance *InGameInstance, class USimpleSaveFile *InFile, float& OutTimeSkip, bool InTriggerPostLevelChange);

	//
private:

	UPROPERTY(VisibleAnywhere, Category="Runtime")
	int32 Progress = 0;

	UPROPERTY(VisibleAnywhere, Category = "Runtime")
	bool TriggerPostLevelChange;

	UPROPERTY(VisibleAnywhere, Category = "Runtime")
	class USaveGameInstance *GameInstance = NULL;

	UPROPERTY(VisibleAnywhere, Category = "Runtime")
	class USimpleSaveFile *File = NULL;

	UPROPERTY(VisibleAnywhere, Category = "Runtime")
	TMap<FName, class AActor*> CustomTags;

	UPROPERTY(VisibleAnywhere, Category = "Runtime")
	bool bLevelChange;

	UPROPERTY(VisibleAnywhere, Category = "Runtime")
	int32 ActorsRestoreStart;

	UPROPERTY(VisibleAnywhere, Category = "Runtime")
	int32 ObjectsRestoreStart;
};
