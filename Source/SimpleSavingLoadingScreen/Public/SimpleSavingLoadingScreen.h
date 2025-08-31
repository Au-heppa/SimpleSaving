#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

//=========================================================================================================
// 
//=========================================================================================================
class ISimpleSavingLoadingScreenModule : public IModuleInterface
{
public:
	/** Loads the module so it can be turned on */
	static inline ISimpleSavingLoadingScreenModule &Get()
	{
		return FModuleManager::LoadModuleChecked<ISimpleSavingLoadingScreenModule>("SimpleSavingLoadingScreen");
	}

	/** Kicks off the loading screen for in game loading (not startup) */
	virtual void StartInGameLoadingScreen(bool bPlayUntilStopped, float PlayTime, bool Play = false) = 0;

	//
	virtual void WaitForMovieToFinish() = 0;

	//
	virtual void SetIcons(TArray<class UMaterialInterface*> InIcons) = 0;

	//
	virtual void SetLoadingScreenStatus(const FText &InText) = 0;
	virtual FText GetLoadingScreenStatus() const = 0;

	/** Stops the loading screen */
	virtual void StopInGameLoadingScreen() = 0;
};
