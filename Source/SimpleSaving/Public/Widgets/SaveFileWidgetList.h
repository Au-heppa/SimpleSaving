// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SaveFileWidgetList.generated.h"

//=================================================================
// 
//=================================================================
UCLASS(Blueprintable, placeable)
class SIMPLESAVING_API USaveFileWidgetList : public UUserWidget
{
	GENERATED_BODY()
	
public:

	//
	virtual void NativeDestruct() override;

	//
	UFUNCTION(BlueprintCallable)
	void DestroyList();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Update();
	virtual void Update_Implementation() { }

	//
	UFUNCTION(BlueprintCallable)
	bool UpdateList(TSoftClassPtr<class USaveFileWidget> InClass, class UPanelWidget *InParent);

	//
	UFUNCTION(BlueprintNativeEvent)
	void OnFileClicked(const FString &InFilename);
	virtual void OnFileClicked_Implementation(const FString &InFilename);

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSaveListWidgetEvent, const FString &, InFilename);

	//
	UPROPERTY(BlueprintAssignable)
	FSaveListWidgetEvent OnFileClick;

private:

	UPROPERTY()
	TArray<class USaveFileWidget*> Widgets;

	//
	UPROPERTY(Category="Runtime", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TWeakObjectPtr<class USaveGameInstance> GameInstance;

	//
	UPROPERTY(Category="Runtime", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TWeakObjectPtr<class APlayerController> PlayerController;
};
