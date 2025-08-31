// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Saving/SaveFileList.h"
#include "SaveFileWidget.generated.h"

//=================================================================
// 
//=================================================================
UCLASS(Blueprintable)
class SIMPLESAVING_API USaveFileWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	//
	void Setup(const FSaveFileList &InFile);

	//
	UFUNCTION(BlueprintNativeEvent)
	void OnSetup();
	virtual void OnSetup_Implementation() { }

	//
	FORCEINLINE const FString &GetFilename() const { return Filename; }

	//
	UFUNCTION(BlueprintCallable)
	void Click();

	//
	UFUNCTION(BlueprintNativeEvent)
	void OnClicked();
	virtual void OnClicked_Implementation() { }

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSaveWidgetEvent, const FString &, InFilename);

	//
	UPROPERTY(BlueprintAssignable)
	FSaveWidgetEvent OnClick;

	//
private:

	//
	UPROPERTY(Category="Runtime", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	FString Filename;

	//
	UPROPERTY(Category="Runtime", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	FDateTime DateTime;
};
