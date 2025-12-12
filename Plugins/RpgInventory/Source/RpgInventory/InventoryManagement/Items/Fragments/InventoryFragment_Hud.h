// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "InventoryFragment_Hud.generated.h"

/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInventoryFragment_Hud : public UInventoryItemFragment
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UTexture2D* GetIcon() const;
	TSoftObjectPtr<UTexture2D> GetIconSoft() const;
private:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSoftObjectPtr<UTexture2D> Icon;
};
