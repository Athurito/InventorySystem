// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "InteractPromptWidget.generated.h"

class UTextBlock;
class UImage;

struct FInteractDisplayData;
/**
 * Prompt widget that can be used directly or subclassed in BP.
 * If the BP contains widgets named TitleText, ActionText and IconImage,
 * they will auto-bind via BindWidget and be updated by SetPromptData.
 */
UCLASS()
class RPGINVENTORY_API UInteractPromptWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// Native setup that applies data to bound widgets
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void SetPromptData(const FInteractDisplayData& Data);

	// Show/Hide the whole prompt widget
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void SetPromptVisible(bool bVisible);

protected:
	// Optional bound widgets (create them in the UMG BP with the same names)
	UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* TitleText = nullptr;
	UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* ActionText = nullptr;
	UPROPERTY(meta=(BindWidgetOptional)) UImage* IconImage = nullptr;
};
