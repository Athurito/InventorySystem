// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "InventoryTabButton.generated.h"

class UCommonLazyImage;
class UCommonTextBlock;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInventoryTabButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly)
	TObjectPtr<UCommonTextBlock> Label = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UCommonLazyImage> Icon = nullptr;

	UFUNCTION(BlueprintCallable)
	void SetLabelAndIcon(const FText& InText, UTexture2D* InIcon);
	
};
