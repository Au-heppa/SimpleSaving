// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Saving/SimpleRestoreHandler.h"
#include "Saving/SimpleSaveFile.h"

//=================================================================
// 
//=================================================================
ASimpleRestoreHandler::ASimpleRestoreHandler()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

//=================================================================
// 
//=================================================================
void ASimpleRestoreHandler::CreateRestoreHandler(class UObject *WorldContext, class USaveGameInstance* InGameInstance, class USimpleSaveFile* InFile, float& OutTimeSkip, bool InTriggerPostLevelChange)
{
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FTransform Transform;


	class ASimpleRestoreHandler *pHandler = Cast<ASimpleRestoreHandler>(WorldContext->GetWorld()->SpawnActor(ASimpleRestoreHandler::StaticClass(), &Transform, Parameters));
	if (IsValid(pHandler))
	{
		pHandler->Initialize(InGameInstance, InFile, OutTimeSkip, InTriggerPostLevelChange);
	}
}

//=================================================================
// 
//=================================================================
void ASimpleRestoreHandler::Initialize(class USaveGameInstance* InGameInstance, class USimpleSaveFile* InFile, float& OutTimeSkip, bool InTriggerPostLevelChange)
{
	GameInstance = InGameInstance;
	File = InFile;
	TriggerPostLevelChange = InTriggerPostLevelChange;

	ActorsRestoreStart = 0;
	ObjectsRestoreStart = 0;
	Progress = 0;

	PrimaryActorTick.SetTickFunctionEnable(true);

	CustomTags.Reset();
	InFile->HandleRestore_BasicObjects(this, OutTimeSkip, CustomTags);
}



