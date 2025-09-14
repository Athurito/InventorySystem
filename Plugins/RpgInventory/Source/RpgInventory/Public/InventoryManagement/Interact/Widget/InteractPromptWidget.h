// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "InteractPromptWidget.generated.h"

struct FInteractDisplayData;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInteractPromptWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category="Interaction")
	void SetPromptData(const FInteractDisplayData& Data);

	// Optional: kleine State-Hilfen
	UFUNCTION(BlueprintImplementableEvent, Category="Interaction")
	void SetPromptVisible(bool bVisible);
};
