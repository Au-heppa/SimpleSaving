// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Saving/SimpleSavedActor.h"

//=================================================================================================
// 
//=================================================================================================
ASimpleSavedActor::ASimpleSavedActor()
{

}

//=================================================================================================
// 
//=================================================================================================
void ASimpleSavedActor::OnRestore(class USaveGameInstance *InGameInstance, class APlayerController *InController)
{
	OnRestored();
}


