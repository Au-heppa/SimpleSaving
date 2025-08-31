// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Saving/SimpleSaveHeader.h"
#include "Kismet/GameplayStatics.h"
#include "Saving/SaveGameInstance.h"
#include "UnrealClient.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "HAL/FileManager.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"

//=================================================================
// 
//=================================================================
FORCEINLINE static int32 FindDataForFile(const TArray<FSimpleHeaderData> &InData, const FString &InFilename)
{
	for (int32 i=0; i<InData.Num(); i++)
	{
		if (InData.GetData()[i].Filename.Equals(InFilename, ESearchCase::IgnoreCase))
			return i;
	}

	return INDEX_NONE;
}

//=================================================================
// 
//=================================================================
static const FString HeaderFilename = TEXT("Header/Header");

//=================================================================
// 
//=================================================================
bool USimpleSaveHeader::SaveHeaderDataFor(const class UObject* WorldContext, const FString& Filename)
{
	//
	class USaveGameInstance* pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveHeader::SaveHeaderDataFor: No game instance!"));
		return false;
	}

	class USimpleSaveHeader* pHeader = GetHeaderData(WorldContext);
	if (!IsValid(pHeader))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveHeader::SaveHeaderDataFor: Failed to get header data!"));
		return false;
	}

	FSimpleHeaderData Data;
	bool bRequestScreenshot = false;
	if (pGameInstance->GatherHeaderData(WorldContext, Data, bRequestScreenshot))
	{
		pGameInstance->GatherLevelChangeTags(WorldContext, Data.LevelChangeTags);

		int32 iIndex = FindDataForFile(pHeader->Data, Filename);
		if (iIndex == INDEX_NONE)
		{
			iIndex = pHeader->Data.Add(Data);
			pHeader->Data.GetData()[iIndex].Filename = Filename;
		}
		else
		{
			pHeader->Data.GetData()[iIndex] = Data;
		}

		pHeader->Data.GetData()[iIndex].MapName = UGameplayStatics::GetCurrentLevelName(WorldContext);

		UGameplayStatics::SaveGameToSlot(pHeader, HeaderFilename, 0);

		if (bRequestScreenshot)
		{
			pHeader->Screenshot(Filename);
		}
		return true;
	}

	return false;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveHeader::CopyHeaderData(const class UObject* WorldContext, const FString& Source, const FString& Destination)
{
	//
	class USaveGameInstance* pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveHeader::CopyHeaderData: No game instance!"));
		return false;
	}

	class USimpleSaveHeader *pHeader = GetHeaderData(WorldContext);
	if (!IsValid(pHeader))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveHeader::CopyHeaderData: No game instance!"));
		return false;
	}

	if (pHeader->FilesRequestingScreenshot.Contains(Source))
	{
		pHeader->FilesRequestingScreenshot.AddUnique(Destination);
	}

	int32 iSource = FindDataForFile(pHeader->Data, Source);
	if (iSource == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveHeader::CopyHeaderData: No game instance!"));
		return false;
	}

	int32 iDestination = FindDataForFile(pHeader->Data, Source);
	if (iDestination == INDEX_NONE)
	{
		iDestination = pHeader->Data.Add(pHeader->Data.GetData()[iSource]);
	}

	pHeader->Data.GetData()[iDestination].Filename = Source;

	UGameplayStatics::SaveGameToSlot(pHeader, HeaderFilename, 0);
	return true;
}

//=================================================================
// 
//=================================================================
bool USimpleSaveHeader::GetHeaderDataFor(const class UObject* WorldContext, const FString& Filename, FSimpleHeaderData& OutData)
{
	//
	class USaveGameInstance* pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveHeader::GetHeaderDataFor: No game instance!"));
		return false;
	}

	class USimpleSaveHeader* pHeader = GetHeaderData(WorldContext);
	if (!IsValid(pHeader))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveHeader::GetHeaderDataFor: No header data!"));
		return false;
	}

	int32 iSource = FindDataForFile(pHeader->Data, Filename);
	if (iSource == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveHeader::GetHeaderDataFor: Data not found for \"%s\"!"), *Filename);
		return false;
	}

	OutData = pHeader->Data.GetData()[iSource];
	return true;
}

//=================================================================
// 
//=================================================================
class USimpleSaveHeader* USimpleSaveHeader::GetHeaderData(const class UObject* WorldContext)
{
	//
	class USaveGameInstance* pGameInstance = Cast<USaveGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
	if (!IsValid(pGameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("USimpleSaveHeader::GetHeaderData: No game instance!"));
		return NULL;
	}

	//If already exists
	if (pGameInstance->SaveHeaderData)
	{
		return pGameInstance->SaveHeaderData;
	}

	//Load if exists
	pGameInstance->SaveHeaderData = Cast<USimpleSaveHeader>(UGameplayStatics::LoadGameFromSlot(HeaderFilename, 0));
	if (pGameInstance->SaveHeaderData)
	{
		return pGameInstance->SaveHeaderData;
	}

	//Create new
	pGameInstance->SaveHeaderData = Cast<USimpleSaveHeader>(UGameplayStatics::CreateSaveGameObject(USimpleSaveHeader::StaticClass()));
	return pGameInstance->SaveHeaderData;
}

//=================================================================
// 
//=================================================================
void USimpleSaveHeader::Screenshot(const FString &Filename)
{
	if (FilesRequestingScreenshot.Num() == 0)
	{
		GEngine->GameViewport->OnScreenshotCaptured().AddUObject(this, &USimpleSaveHeader::AcceptScreenshot);
	}

	FilesRequestingScreenshot.AddUnique(Filename);
	FScreenshotRequest::RequestScreenshot(false); // False means don't include any UI
}

//=================================================================
// 
//=================================================================
static FColor GetTotalColor(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData, int32 x, int32 y, float offset, int32 count)
{
	FLinearColor TotalColor = FLinearColor::Black;

	for (int32 x0=0; x0<count; x0++)
	{
		float x1 = (x * offset * count) + x0 * offset;
		int32 prev_x = FMath::Clamp(x1, 0, InSizeX - 1);
		int32 next_x = FMath::Clamp(prev_x+1, 0, InSizeX-1);
		float frac_x = x1 - prev_x;

		for (int32 y0=0; y0<count; y0++)
		{
			float y1 = (y * offset * count) + y0 * offset;

			int32 prev_y = FMath::Clamp(y1, 0, InSizeY - 1);
			int32 next_y = FMath::Clamp(prev_y+1, 0, InSizeY-1);
			float frac_y = y1 - prev_y;

			FLinearColor Upper = ((1 - frac_x) * InImageData.GetData()[(prev_y * InSizeX) + prev_x].ReinterpretAsLinear()) + (frac_x * InImageData.GetData()[(prev_y * InSizeX) + next_x].ReinterpretAsLinear());
			FLinearColor Bottom = ((1 - frac_x) * InImageData.GetData()[(next_y * InSizeX) + prev_x].ReinterpretAsLinear()) + (frac_x * InImageData.GetData()[(next_y * InSizeX) + next_x].ReinterpretAsLinear());

			TotalColor += ((1.0f - frac_y) * Upper) + (frac_y * Bottom);
		}
	}

	TotalColor = TotalColor / (float)(count * count);

	return TotalColor.ToFColor(false);
}

//=================================================================
// 
//=================================================================
void USimpleSaveHeader::AcceptScreenshot(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData)
{
	if (FilesRequestingScreenshot.Num() == 0)
		return;

	TArray<FColor> ResizedData;

	int32 TargetWide = 0;
	int32 TargetTall = 200;
	if (InSizeY > TargetTall)
	{
		float flFraction = (float)InSizeX / (float)InSizeY;

		TargetWide = flFraction * TargetTall;

		float flPerPixel = (float)InSizeY / (float)TargetTall;

		int32 nCount = 1 + flPerPixel;

		float flOffset = flPerPixel / nCount;

		//UE_LOG(LogTemp, Display, TEXT("Count %d offset %.2f length %.2f per pixel %.2f"), nCount, flOffset, flOffset * nCount, flPerPixel);

		ResizedData.SetNum( TargetTall * TargetWide );
	
		//
		for (int32 x=0; x<TargetWide; x++)
		{
			for (int32 y=0; y<TargetTall; y++)
			{
				int32 target = (y * TargetWide) + x;
				ResizedData.GetData()[target] = GetTotalColor(InSizeX, InSizeY, InImageData, x, y, flOffset, nCount);
			}
		}
	}
	else
	{
		TargetWide = InSizeX;
		TargetTall = InSizeY;

		ResizedData = InImageData;
	}

	// Load image wrapper module
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);

	// Provide image wrapper with raw pixels (TArray<FColor>)
	ImageWrapper->SetRaw(&ResizedData[0], ResizedData.Num() * sizeof(FColor), TargetWide, TargetTall, ERGBFormat::BGRA, 8);

	// Get the image using 90% JPEG compression
	const TArray64<uint8>& PNGData = ImageWrapper->GetCompressed(100);

	IFileManager* FileManager = &IFileManager::Get();

	//Save for each save file that is currently requesting
	for (int32 i=0; i< FilesRequestingScreenshot.Num(); i++)
	{
		FString Filename = FPaths::ProjectSavedDir() + FString::Printf(TEXT("SaveGames/Header/%s.jpg"), *FilesRequestingScreenshot.GetData()[i]);

		FArchive* Ar = FileManager->CreateFileWriter(Filename.GetCharArray().GetData());
		if (Ar != nullptr)
		{
			Ar->Serialize((void*)PNGData.GetData(), PNGData.GetAllocatedSize());
			delete Ar;

			UE_LOG(LogTemp, Display, TEXT("Saved screenshot: %s"), *Filename);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to create file writer!"));
		}
	}

	FilesRequestingScreenshot.Reset();
	GEngine->GameViewport->OnScreenshotCaptured().RemoveAll(this);
}