// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "Rpg_FragmentTags.h"
#include "InventoryFragment_Stackable.generated.h"

/**
 * 
 */
UCLASS()
class UInventoryFragment_Stackable : public UInventoryItemFragment
{
	GENERATED_BODY()
public:
	
	int32 GetMaxStackSize() const { return MaxStackSize; }
	virtual void OnStackInitialized(UInventoryItemInstance* Instance, int32 StackCount) const override;

private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 MaxStackSize{1};
};
