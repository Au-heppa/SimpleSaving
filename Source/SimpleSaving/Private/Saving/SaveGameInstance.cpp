// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Saving/SaveGameInstance.h"
#include "Saving/SimpleSaveFile.h"
#include "Kismet/GameplayStatics.h"
#include "SimpleSavingLoadingScreen.h"
#include "Runtime/Core/Public/GenericPlatform/GenericPlatformFile.h"
#include "Saving/SimpleSaveHeader.h"
#include "Saving/SaveInterface.h"
#include "Saving/SimpleRestoreHandler.h"

//==============================================================================================================
//
//==============================================================================================================
USaveGameInstance::USaveGameInstance(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
	LevelChangeFilename = TEXT("LevelChange");
	LoadGame = NULL;

	SaveFilename = TEXT("SaveGame");
}

//=================================================================
// 
//=================================================================
void USaveGameInstance::StartLoading(class USimpleSaveFile *InLoadGame)
{
	LoadGame = InLoadGame;
	bIsLoading = true;
	bInLevelChange = false;
	UE_LOG(LogTemp, Display, TEXT("bInLevelChange set to false by StartLoading!"));
}

//=================================================================
// 
//=================================================================
void USaveGameInstance::FinishLoading(class UObject *WorldContextObject)
{
	if (IsLoading() || InLevelChange())
	{
		LoadGame->FinishLoading();
		LoadGame->ClearCurrentLevelAndGlobalData(WorldContextObject);
		//LoadGame = NULL;
	}

	bIsLoading = false;
	bInLevelChange = false;
	UE_LOG(LogTemp, Display, TEXT("bInLevelChange set to false by FinishLoading!"));

	if (!OnRestoreFinished.IsBound() && UseRestoreHandler)
	{
		ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
		LoadingScreenModule.SetLoadingScreenStatus(FText::FromString(TEXT("OnRestoreFinished not bound!")));
	}

	OnRestoreFinished.Broadcast();
}

#if WITH_EDITOR

//=================================================================
// 
//=================================================================
bool USaveGameInstance::CheckSuccessLoad(class UObject *WorldContextObject, bool InMultiLevel)
{
	if (!GetLoadGame())
	{
		UE_LOG(LogTemp, Error, TEXT("CheckSuccessLoad: No load game!"));
		return false;
	}

	DebugSaveFile = Cast<USimpleSaveFile>(UGameplayStatics::CreateSaveGameObject(USimpleSaveFile::StaticClass()));
	if (!DebugSaveFile)
	{
		UE_LOG(LogTemp, Fatal, TEXT("CheckSuccessLoad: Failed to create debug save file!"));
		return false;
	}

	if (DebugSaveFile->DebugSaveGame(WorldContextObject, InMultiLevel))
	{
		UE_LOG(LogTemp, Display, TEXT("CheckSuccessLoad: Checking load success returned true!"));
		return true;
	}

	UE_LOG(LogTemp, Fatal, TEXT("CheckSuccessLoad: Checking load success returned false!"));
	return false;
}

#endif //

//=================================================================
// 
//=================================================================
bool USaveGameInstance::HandleRestore(class UObject *WorldContextObject, bool StopLoadingScreen, float &OutTimeSkip, bool TriggerPostLevelChange)
{
	OutTimeSkip = 0.0f;

	bool bRestore = false;
	if (IsLoading() || InLevelChange())
	{
		if (UseRestoreHandler)
		{
			ASimpleRestoreHandler::CreateRestoreHandler(WorldContextObject, this, LoadGame, OutTimeSkip, TriggerPostLevelChange);
			return true;
		}
		else
		{
			bRestore = true;
			LoadGame->HandleRestore(WorldContextObject, OutTimeSkip, TriggerPostLevelChange);
		}
	}

	//Intentionally negative so that if the game gets save one second from Now, then the value will be
	//-Now + (1.0f + Now) = 1.0f from GetTotalTime()
	TotalTime = -UGameplayStatics::GetTimeSeconds(this);

	FinishLoading(WorldContextObject);

	if (StopLoadingScreen)
	{
		DoStopLoadingScreen();
	}

	return bRestore;
}

//=================================================================
// 
//=================================================================
void USaveGameInstance::DoStopLoadingScreen()
{
	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
	LoadingScreenModule.StopInGameLoadingScreen();
}

//=================================================================
// 
//=================================================================
void USaveGameInstance::SetSaveGame(class USimpleSaveFile *InSaveGame)
{
	LoadGame = InSaveGame;
	bIsLoading = false;
}

//=================================================================
// 
//=================================================================
bool USaveGameInstance::PreChangeLevel(class UObject *WorldContextObject, const TSoftObjectPtr<class UWorld> &InLevel, FGameplayTag InPositionTag)
{
	TArray<class AActor*> AllActors;
	UGameplayStatics::GetAllActorsWithInterface(WorldContextObject, USaveInterface::StaticClass(), AllActors);

	//Go through all the actors
	for (int32 i = AllActors.Num() - 1; i >= 0; i--)
	{
		ISaveInterface* pInterface = Cast<ISaveInterface>(AllActors.GetData()[i]);
		if (!pInterface)
		{
			continue;
		}

		if (pInterface->ShouldRespawnOnLevelChange())
		{
			AllActors.GetData()[i]->Destroy();
		}
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool USaveGameInstance::ChangeLevel(class UObject *WorldContextObject, TSoftObjectPtr<class UWorld> InLevel, FGameplayTag InPositionTag)
{
	if (!IsValid(WorldContextObject))
	{
		UE_LOG(LogTemp, Fatal, TEXT("No world context object!"));
		return false;
	}

	if (IsLoading()) 
	{
		UE_LOG(LogTemp, Fatal, TEXT("Trying to change level while loading!"));
		return false;
	}

	if (LevelChangeFilename.Len() == 0)
	{
		UE_LOG(LogTemp, Fatal, TEXT("LevelChangeFilename is None!"));
		return false;
	}

	if (!PreChangeLevel(WorldContextObject, InLevel, InPositionTag))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Failed PreChanceLevel!"));
		return false;
	}

#if WITH_EDITOR
	TArray<class AActor*> AllActors;
	UGameplayStatics::GetAllActorsWithInterface(WorldContextObject, USaveInterface::StaticClass(), AllActors);
	for (int32 i=0; i<AllActors.Num(); i++)
	{
		class ISaveInterface *pInterface = Cast<ISaveInterface>(AllActors.GetData()[i]);
		if (pInterface && pInterface->BlockSaving())
		{
			UE_LOG(LogTemp, Fatal, TEXT("Saving is blocked by %s with reason %s"), *AllActors.GetData()[i]->GetName(), *pInterface->GetBlockSavingReason());
		}
	}
#endif 


	if (USimpleSaveFile::SaveGame(WorldContextObject, LevelChangeFilename, true, true) && LoadGame)
	{
		bInLevelChange = true;
		UE_LOG(LogTemp, Display, TEXT("bInLevelChange set to true!"));
		bIsLoading = false;
		LevelChangePositionTag = InPositionTag;

		FGameplayTagContainer LevelChangeTags;
		GatherLevelChangeTags(WorldContextObject, LevelChangeTags);

		SetLoadingScreenIconsFromLevelChangeTags(LevelChangeTags);

		ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
		LoadingScreenModule.StartInGameLoadingScreen(true, 1.0f);

		GEngine->ForceGarbageCollection(true);
		UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContextObject, InLevel);
		return true;
	}

	UE_LOG(LogTemp, Fatal, TEXT("Failed to save game!"));
	return false;
}

//=================================================================
// 
//=================================================================
void USaveGameInstance::SetLoadingScreenIconsFromLevelChangeTags(const FGameplayTagContainer &InContainer)
{
	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();

	TArray<class UMaterialInterface*> Icons;
	TArray<FGameplayTag> Tags;
	InContainer.GetGameplayTagArray(Tags);
	for (int32 i=0; i<Tags.Num(); i++)
	{
		if (LevelChangeIcons.Contains(Tags.GetData()[i]))
		{
			Icons.Add(LevelChangeIcons[Tags.GetData()[i]]);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("USaveGameInstance: Setting %d icons!"), Icons.Num());

	LoadingScreenModule.SetIcons(Icons);
}

//=================================================================
// 
//=================================================================
bool USaveGameInstance::StartNewGame(class UObject *WorldContextObject, TSoftObjectPtr<class UWorld> InLevel, FGameplayTagContainer InLevelChangeTags)
{
	if (!IsValid(WorldContextObject))
	{
		UE_LOG(LogTemp, Fatal, TEXT("No world context!"));
		return false;
	}

	bInLevelChange = false;
	bIsLoading = false;
	LoadGame = NULL;

	OnNewGame(WorldContextObject, false);

	SetLoadingScreenIconsFromLevelChangeTags(InLevelChangeTags);

	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
	LoadingScreenModule.StartInGameLoadingScreen(true, 1.0f);

	if (!InLevel.IsNull())
	{
		//UE_LOG(LogTemp, Fatal, TEXT("Opening level %s"), *InLevel.ToString());
		GEngine->ForceGarbageCollection(true);
		UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContextObject, InLevel);
		return true;
	}

	UE_LOG(LogTemp, Fatal, TEXT("Level is NULL!"));
	return false;
}

//=================================================================
// 
//=================================================================
void USaveGameInstance::DestroySaveFileList()
{
	SaveFiles.Reset();
}

//=================================================================
// 
//=================================================================
FText USaveGameInstance::GetLoadingScreenText()
{
	ISimpleSavingLoadingScreenModule& LoadingScreenModule = ISimpleSavingLoadingScreenModule::Get();
	return LoadingScreenModule.GetLoadingScreenStatus();
}

//=================================================================
// 
//=================================================================
class FFindSavesVisitor : public IPlatformFile::FDirectoryVisitor
{
public:
	FFindSavesVisitor() {}

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
	{
		if (!bIsDirectory)
		{
			FString FullFilePath(FilenameOrDirectory);
			if (FPaths::GetExtension(FullFilePath) == TEXT("sav"))
			{
				FString CleanFilename = FPaths::GetBaseFilename(FullFilePath);
				CleanFilename = CleanFilename.Replace(TEXT(".sav"), TEXT(""));
				SavesFound.Add(CleanFilename);
			}
		}
		return true;
	}
	TArray<FString> SavesFound;
};

//=================================================================
// 
//=================================================================
FORCEINLINE FString GetFullFilename(const FString &InSavesFolder, const FString &InFilename)
{
	return FString::Printf(TEXT("%s/%s.sav"), *InSavesFolder, *InFilename);
}

//=================================================================
// 
//=================================================================
bool USaveGameInstance::GenerateSaveFileList()
{
	if (SaveFiles.Num() > 0)
		return false;

	//Folder
	const FString SavesFolder = FPaths::ProjectSavedDir() + TEXT("SaveGames");

	FFindSavesVisitor Visitor;
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SavesFolder, Visitor);

	SaveFiles.SetNum(Visitor.SavesFound.Num());

	//
	for (int32 i=0; i<SaveFiles.Num(); i++)
	{
		SaveFiles.GetData()[i].Filename = Visitor.SavesFound.GetData()[i];
		SaveFiles.GetData()[i].DateTime = FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*GetFullFilename(SavesFolder, SaveFiles.GetData()[i].Filename));
	}

	SortSaveFiles();

	return SaveFiles.Num() > 0;
}

//=================================================================
// 
//=================================================================
bool USaveGameInstance::GenerateSaveFilesListEditor(TArray<FString>& OutFiles)
{
	//Folder
	const FString SavesFolder = FPaths::ProjectSavedDir() + TEXT("SaveGames");

	FFindSavesVisitor Visitor;
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SavesFolder, Visitor);

	TArray< FSaveFileList> List;
	List.SetNum(Visitor.SavesFound.Num());

	//
	for (int32 i = 0; i < List.Num(); i++)
	{
		List.GetData()[i].Filename = Visitor.SavesFound.GetData()[i];
		List.GetData()[i].DateTime = FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*GetFullFilename(SavesFolder, List.GetData()[i].Filename));
	}

	//Sort the files
	List.Sort([](const FSaveFileList& A, const FSaveFileList& B)
		{
			return A.DateTime > B.DateTime;
		});

	OutFiles.SetNum(List.Num());
	for (int32 i=0; i<List.Num(); i++)
	{
		OutFiles.GetData()[i] = List.GetData()[i].Filename;
	}

	return OutFiles.Num() > 0;
}

//=================================================================
// 
//=================================================================
void USaveGameInstance::SortSaveFiles()
{
	//Sort the files
	SaveFiles.Sort([](const FSaveFileList & A, const FSaveFileList & B)
	{
		return A.DateTime > B.DateTime;
	});
}

//=================================================================
// 
//=================================================================
bool USaveGameInstance::UpdateSaveFile(const FString &InFilename)
{
	if (SaveFiles.Num() == 0)
	{
		GenerateSaveFileList();
		return true;
	}

	const FString SavesFolder = FPaths::ProjectSavedDir() + TEXT("SaveGames");

	//Find if this save file already exists
	for (int32 i=0; i<SaveFiles.Num(); i++)
	{
		if (SaveFiles.GetData()[i].Filename.Equals(InFilename, ESearchCase::IgnoreCase))
		{
			SaveFiles.GetData()[i].DateTime = FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*GetFullFilename(SavesFolder, InFilename));

			//UE_LOG(LogTemp, Display, TEXT("Save file \"%s\" date time \"%s\""), *InFilename, *SaveFiles.GetData()[i].DateTime.ToString());
			return true;
		}
	}

	FSaveFileList NewFile;
	NewFile.Filename = InFilename;
	NewFile.DateTime = FPlatformFileManager::Get().GetPlatformFile().GetTimeStamp(*GetFullFilename(SavesFolder, InFilename));
	SaveFiles.Add(NewFile);

	//UE_LOG(LogTemp, Display, TEXT("Save file \"%s\" date time \"%s\""), *InFilename, *NewFile.DateTime.ToString());

	SortSaveFiles();

	return true;
}
