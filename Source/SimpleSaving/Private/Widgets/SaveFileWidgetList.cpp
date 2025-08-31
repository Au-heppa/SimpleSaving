// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network


#include "Widgets/SaveFileWidgetList.h"
#include "UMG/Public/Blueprint/WidgetBlueprintLibrary.h"
#include "UMG/Public/Components/PanelWidget.h"

//=================================================================
// 
//=================================================================
void USaveFileWidgetList::NativeDestruct()
{
	DestroyList();

	Super::NativeDestruct();
}

//=================================================================
// 
//=================================================================
void USaveFileWidgetList::DestroyList()
{
	//
	for (int32 i=0; i<Widgets.Num(); i++)
	{
		if (IsValid(Widgets.GetData()[i]))
		{
			Widgets.GetData()[i]->RemoveFromParent();
		}
	}

	Widgets.Reset();
}

//=================================================================
// 
//=================================================================
void USaveFileWidgetList::OnFileClicked_Implementation(const FString &InFilename)
{
	OnFileClick.Broadcast(InFilename);
}

//=================================================================
// 
//=================================================================
bool USaveFileWidgetList::UpdateList(TSoftClassPtr<class USaveFileWidget> InClass, class UPanelWidget *InParent)
{
	class USaveGameInstance *pGameInstance = GameInstance.Get();
	if (!IsValid(pGameInstance))
	{
		pGameInstance = GetGameInstance<USaveGameInstance>();

		if (!IsValid(pGameInstance))
		{
			return false;
		}

		GameInstance = pGameInstance;
	}

	class APlayerController *pController = PlayerController.Get();
	if (!IsValid(pController))
	{
		pController = UGameplayStatics::GetPlayerController(this, 0);
		
		if (!IsValid(pController))
		{
			return false;
		}

		PlayerController = pController;
	}

	pGameInstance->GenerateSaveFileList();

	TMap<FString, class USaveFileWidget*> CachedWidgets;

	bool bFilesMissing = false;

	//Figure out if each file has a widget already or not
	const TArray<FSaveFileList> &List = pGameInstance->GetSaveFiles();
	for (int32 i=0; i<List.Num(); i++)
	{
		const FSaveFileList &File = List.GetData()[i];

		class USaveFileWidget *pWidget = NULL;
		for (int32 j=0; j<Widgets.Num(); j++)
		{
			if (Widgets.GetData()[j]->GetFilename().Equals(File.Filename, ESearchCase::IgnoreCase))
			{
				pWidget = Widgets.GetData()[j];
				break;
			}
		}

		if (pWidget)
		{
			CachedWidgets.Emplace(File.Filename, pWidget);
		}
		else
		{
			bFilesMissing = true;
		}
	}

	//If any files missing, need to reorganize
	if (bFilesMissing)
	{
		TSubclassOf<class USaveFileWidget> WidgetClass = InClass.IsPending() ? InClass.LoadSynchronous() : InClass.Get();
		if (!WidgetClass)
			return false;

		//
		for (int32 i=Widgets.Num()-1; i>=0; i--)
		{
			if (!IsValid(Widgets.GetData()[i]))
			{
				Widgets.RemoveAt(i);
				continue;
			}

			//Check if no longer in list
			if (!CachedWidgets.Contains(Widgets.GetData()[i]->GetFilename()))
			{
				Widgets.GetData()[i]->RemoveFromParent();
				Widgets.RemoveAt(i);
				continue;
			}

			//Unparent
			Widgets.GetData()[i]->RemoveFromParent();
		}

		//Now create and reorganize the files
		for (int32 i=0; i<List.Num(); i++)
		{
			class USaveFileWidget *pWidget = NULL;
			
			if (CachedWidgets.Contains(List.GetData()[i].Filename))
			{
				pWidget = CachedWidgets[List.GetData()[i].Filename];
			}
			else
			{
				pWidget = Cast<USaveFileWidget>(UWidgetBlueprintLibrary::Create(pController, WidgetClass, pController));

				//Start listening to event
				if (IsValid(pWidget))
				{
					Widgets.Add(pWidget);
					pWidget->OnClick.AddDynamic(this, &USaveFileWidgetList::OnFileClicked);
				}
			}

			if (IsValid(pWidget))
			{
				InParent->AddChild(pWidget);
				pWidget->Setup(List.GetData()[i]);
			}
		}
	}

	return Widgets.Num() > 0;
}