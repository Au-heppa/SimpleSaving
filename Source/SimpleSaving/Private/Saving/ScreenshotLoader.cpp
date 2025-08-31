// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network


#include "Saving/ScreenshotLoader.h"
#include "UMG/Public/Blueprint/UserWidget.h"
#include "Saving/SimpleSaveFile.h"


//===========================================================================================================================
//
//===========================================================================================================================
UScreenshotLoader::UScreenshotLoader()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	

	// ...
	ComponentTags.Add(USimpleSaveFile::Name_DontSave);
}

//===========================================================================================================================
//
//===========================================================================================================================
void UScreenshotLoader::OnUnregister()
{
	Super::OnUnregister();

	DestroyRunnable();
}

//===========================================================================================================================
//
//===========================================================================================================================
void UScreenshotLoader::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	DestroyRunnable();
}


//===========================================================================================================================
//
//===========================================================================================================================
void UScreenshotLoader::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

//===========================================================================================================================
//
//===========================================================================================================================
void UScreenshotLoader::InitRunnable()
{
	if (!LoaderRunnable)
	{
		LoaderRunnable = new FScreenshotLoaderRunnable();
	}
}

//===========================================================================================================================
//
//===========================================================================================================================
void UScreenshotLoader::DestroyRunnable()
{
	if (LoaderRunnable)
	{
		LoaderRunnable->EnsureCompletion();
		delete LoaderRunnable;
		LoaderRunnable = NULL;
	}
}

//===========================================================================================================================
//
//===========================================================================================================================
void UScreenshotLoader::HandleReceivedData(int32 Wide, int32 Tall, const TArray<uint8> &UncompressedBGRA)
{
	if (Wide == 0 || Tall == 0 || UncompressedBGRA.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("UScreenshotLoader::HandleReceivedData: Wide %d Tall %d Data size %d"), Wide, Tall, UncompressedBGRA.Num());
		return;
	}

	//If object no longer exists then don't bother
	if (!IsValid(Requests.GetData()[0].LatentInfo.CallbackTarget))
	{
		UE_LOG(LogTemp, Error, TEXT("UScreenshotLoader::HandleReceivedData: Latent object no longer exists!"));
		return;
	}

	//Widgets must still be on screen
	class UUserWidget* pWidget = Cast<UUserWidget>(Requests.GetData()[0].LatentInfo.CallbackTarget);
	if (pWidget)
	{
		if (!pWidget->IsInViewport() && !IsValid((class UObject*)pWidget->GetParent()))
		{
			UE_LOG(LogTemp, Error, TEXT("UScreenshotLoader::HandleReceivedData: Latent object no longer on screen!"));
			return;
		}
	}

	UFunction* pExecutionFunction = Requests.GetData()[0].LatentInfo.CallbackTarget->FindFunction(Requests.GetData()[0].LatentInfo.ExecutionFunction);
	if (!IsValid(pExecutionFunction))
	{
		UE_LOG(LogTemp, Error, TEXT("UScreenshotLoader::HandleReceivedData: Failed to find function %s"), *Requests.GetData()[0].LatentInfo.ExecutionFunction.ToString());
		return;
	}

	class UTexture2D* pTexture = NULL;
	for (int32 i = 0; i < ScreenshotPool.Num(); i++)
	{
		if (ScreenshotPool.GetData()[i]->GetSizeX() == Wide && ScreenshotPool.GetData()[i]->GetSizeY() == Tall)
		{
			pTexture =ScreenshotPool.GetData()[i];
			ScreenshotPool.RemoveAt(i);
			break;
		}
	}

	//Create new
	if (!IsValid(pTexture))
	{
		int32 i = Screenshots.Add(UTexture2D::CreateTransient(Wide, Tall, PF_B8G8R8A8));
		pTexture = Screenshots.GetData()[i];
	}

	void* TextureData = pTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
	pTexture->GetPlatformData()->Mips[0].BulkData.Unlock();

	// Update the rendering resource from data.
	pTexture->UpdateResource();

	*Requests.GetData()[0].TexturePointer = pTexture;
	Requests.GetData()[0].LatentInfo.CallbackTarget->ProcessEvent(pExecutionFunction, &Requests.GetData()[0].LatentInfo.Linkage);

	UE_LOG(LogTemp, Display, TEXT("UScreenshotLoader::HandleReceivedData: Success!"));
}

//===========================================================================================================================
//
//===========================================================================================================================
void UScreenshotLoader::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (LoaderRunnable && LoaderRunnable->CanAddInput())
	{
		if (bStartedLoading)
		{
			HandleReceivedData(LoaderRunnable->Output_Wide, LoaderRunnable->Output_Tall, LoaderRunnable->Output_UncompressedBGRA);

			Requests.RemoveAt(0);
		}

		if (Requests.Num() == 0)
		{
			PrimaryComponentTick.SetTickFunctionEnable(false);
			bStartedLoading = false;
		}
		else
		{
			bStartedLoading = true;
			LoaderRunnable->SetLoadScreenshot(Requests.GetData()[0].Filename);
		}
	}
}

//=================================================================
// 
//=================================================================
class UScreenshotLoader* UScreenshotLoader::GetScreenshotLoader(const class UObject* WorldContext)
{
	class APlayerController *pController = UGameplayStatics::GetPlayerController(WorldContext, 0);
	if (!IsValid(pController))
		return NULL;

	return pController->FindComponentByClass<UScreenshotLoader>();
}

//=================================================================
// 
//=================================================================
void UScreenshotLoader::RequestScreenshot(const class UObject* WorldContext, const FString& Filename, class UTexture2D*& Screenshot, FLatentActionInfo LatentInfo)
{
	class UScreenshotLoader* pScreenshotLoader = UScreenshotLoader::GetScreenshotLoader(WorldContext);
	if (!IsValid(pScreenshotLoader))
	{
		UE_LOG(LogTemp, Error, TEXT("Add UScreenshotLoader component to your PlayerController!"));
		return;
	}

	FRequestHeaderScreenshot Request;
	Request.Filename = Filename;
	Request.LatentInfo = LatentInfo;
	Request.TexturePointer = &Screenshot;
	pScreenshotLoader->Requests.Add(Request);
	pScreenshotLoader->PrimaryComponentTick.SetTickFunctionEnable(true);
	pScreenshotLoader->InitRunnable();
}

//=================================================================
// 
//=================================================================
void UScreenshotLoader::FreeScreenshot(const class UObject* WorldContext, class UTexture2D* Texture)
{
	if (!IsValid(Texture))
		return;

	class UScreenshotLoader *pScreenshotLoader = UScreenshotLoader::GetScreenshotLoader(WorldContext);
	if (pScreenshotLoader && pScreenshotLoader->Screenshots.Contains(Texture))
	{
		pScreenshotLoader->ScreenshotPool.AddUnique(Texture);
	}
}
