// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Saving/ScreenshotLoaderRunnable.h" // Change this to reference the header file above
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

#pragma region Main Thread Code

//=============================================================================================================================
//
//=============================================================================================================================
FScreenshotLoaderRunnable::FScreenshotLoaderRunnable()
{
	bInputReady = false;
	bRunThread = true;

	// Constructs the actual thread object. It will begin execution immediately
	// If you've passed in any inputs, set them up before calling this.
	Thread = FRunnableThread::Create(this, TEXT("FScreenshotLoaderRunnable"));
}

//=============================================================================================================================
//
//=============================================================================================================================
FScreenshotLoaderRunnable::~FScreenshotLoaderRunnable()
{
	if (Thread)
	{
		// Kill() is a blocking call, it waits for the thread to finish.
		// Hopefully that doesn't take too long
		Thread->Kill();
		delete Thread;
	}
}


//=============================================================================================================================
//
//=============================================================================================================================
void FScreenshotLoaderRunnable::EnsureCompletion()
{ 
	Stop(); 
	Thread->WaitForCompletion(); 
}

//=============================================================================================================================
//
//=============================================================================================================================
void FScreenshotLoaderRunnable::SetLoadScreenshot( const FString &InFilename )
{
	if (!bInputReady)
	{
		Input_Filename = InFilename;
		Output_Wide = 0;
		Output_Tall = 0;
		Output_UncompressedBGRA.Reset();

		bInputReady = true;
	}
}

#pragma endregion
// The code below will run on the new thread.

//=============================================================================================================================
//
//=============================================================================================================================
bool FScreenshotLoaderRunnable::Init()
{
	UE_LOG(LogTemp, Display, TEXT("FScreenshotLoaderRunnable: Initialized!"))

	// Return false if you want to abort the thread
	return true;
}

//=============================================================================================================================
//
//=============================================================================================================================
bool FScreenshotLoaderRunnable::LoadScreenshot()
{
	FString ScreenshotFilename = FPaths::ProjectSavedDir() + FString::Printf(TEXT("SaveGames/Header/%s.jpg"), *Input_Filename);

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	// Note: PNG format.  Other formats are supported
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
	if (!ImageWrapper.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FScreenshotLoaderRunnable::LoadScreenshot: Failed to create image wrapped!"));
		return false;
	}

	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *ScreenshotFilename))
	{
		UE_LOG(LogTemp, Error, TEXT("FScreenshotLoaderRunnable::LoadScreenshot: Failed to load file \"%s\""), *ScreenshotFilename);
		return false;
	}

	if (!ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		UE_LOG(LogTemp, Error, TEXT("FScreenshotLoaderRunnable::LoadScreenshot: Failed to compressed data!"));
		return false;
	}

	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, Output_UncompressedBGRA))
	{
		UE_LOG(LogTemp, Error, TEXT("FScreenshotLoaderRunnable::LoadScreenshot: Failed to get raw data!"));
		return false;
	}

	Output_Wide = ImageWrapper->GetWidth();
	Output_Tall = ImageWrapper->GetHeight();
	return true;
}

//=============================================================================================================================
//
//=============================================================================================================================
uint32 FScreenshotLoaderRunnable::Run()
{
	// Peform your processor intensive task here. In this example, a neverending
	// task is created, which will only end when Stop is called.
	while (bRunThread)
	{
		if (bInputReady)
		{
			LoadScreenshot();

			//
			bInputReady = false;
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("My custom thread is running!"))
			FPlatformProcess::Sleep(0.01f);
		}
	}

	return 0;
}

//=============================================================================================================================
//
//=============================================================================================================================
void FScreenshotLoaderRunnable::Stop()
{
	// Clean up memory usage here, and make sure the Run() function stops soon
	// The main thread will be stopped until this finishes!

	// For this example, we just need to terminate the while loop
	// It will finish in <= 1 sec, due to the Sleep()
	bRunThread = false;
}