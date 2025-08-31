// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScreenshotLoaderRunnable.h"
#include "ScreenshotLoader.generated.h"

//==============================================================================================================
// 
//==============================================================================================================
USTRUCT(BlueprintType)
struct FRequestHeaderScreenshot
{
	GENERATED_USTRUCT_BODY()

	//
	UPROPERTY()
	FString Filename;

	//
	UPROPERTY()
	FLatentActionInfo LatentInfo;

	//
	class UTexture2D** TexturePointer = NULL;
};

//=============================================================================================================================
//
//=============================================================================================================================
UCLASS( ClassGroup=(Heroine), meta=(BlueprintSpawnableComponent) )
class SIMPLESAVING_API UScreenshotLoader : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UScreenshotLoader();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//
	virtual void OnUnregister() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	//
	static class UScreenshotLoader *GetScreenshotLoader(const class UObject* WorldContext);

	//
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext", Latent, LatentInfo = "LatentInfo"))
	static void RequestScreenshot(const class UObject* WorldContext, const FString& Filename, class UTexture2D*& Screenshot, FLatentActionInfo LatentInfo);

	//
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"))
	static void FreeScreenshot(const class UObject* WorldContext, class UTexture2D* Texture);

private:

	//
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Runtime", meta = (AllowPrivateAccess = true))
	TArray<class UTexture2D*> Screenshots;

	//
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Runtime", meta = (AllowPrivateAccess = true))
	TArray<class UTexture2D*> ScreenshotPool;

	//
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Runtime", meta = (AllowPrivateAccess = true))
	TArray<FRequestHeaderScreenshot> Requests;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Runtime", meta = (AllowPrivateAccess = true))
	bool bStartedLoading = false;

private:

	//
	void InitRunnable();

	//
	void DestroyRunnable();

	//
	void HandleReceivedData(int32 Wide, int32 Tall, const TArray<uint8>& UncompressedBGRA);

	//
	FScreenshotLoaderRunnable* LoaderRunnable = NULL;
};
