// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "LevelChangeInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class ULevelChangeInterface : public UInterface
{
	GENERATED_BODY()
};

//=================================================================================================
// 
//=================================================================================================
class SIMPLESAVING_API ILevelChangeInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool OnLevelChange(class APlayerController *InController, class APawn *InPawn, const FGameplayTag &InPositionTag);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool PostLevelChange(class APlayerController *InController, class APawn *InPawn);
};
