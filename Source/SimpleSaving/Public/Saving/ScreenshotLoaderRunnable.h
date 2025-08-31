// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

//=============================================================================================================================
//
//=============================================================================================================================
class SIMPLESAVING_API FScreenshotLoaderRunnable : public FRunnable
{
public:

	// Constructor, create the thread by calling this
	FScreenshotLoaderRunnable();

	// Destructor
	virtual ~FScreenshotLoaderRunnable() override;


	// Overriden from FRunnable
	// Do not call these functions youself, that will happen automatically
	bool Init() override; // Do your setup here, allocate memory, ect.
	uint32 Run() override; // Main data processing happens here
	void Stop() override; // Clean up any memory you allocated here


	//=============================================================================================================================
	// INPUT / OUTPUT
	//=============================================================================================================================
public:

	//
	void EnsureCompletion();

	//
	void SetLoadScreenshot(const FString& InFilename);

	//
	FORCEINLINE bool CanAddInput() const { return !bInputReady; }

	//=============================================================================================================================
	//
	//=============================================================================================================================
private:

	//
	FString Input_Filename;

public:

	//
	TArray<uint8> Output_UncompressedBGRA;

	//
	int32 Output_Wide;

	//
	int32 Output_Tall;

private:

	//
	bool LoadScreenshot();

	//=============================================================================================================================
	//
	//=============================================================================================================================
private:

	//
	bool bInputReady;

	// Thread handle. Control the thread using this, with operators like Kill and Suspend
	FRunnableThread* Thread;

	// Used to know when the thread should exit, changed in Stop(), read in Run()
	bool bRunThread;
};