// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "InventoryFragment_Durability.generated.h"

/**
 * Fragment to define durability for an item.
 */
UCLASS()
class RPGINVENTORY_API UInventoryFragment_Durability : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const override;

	UPROPERTY(EditAnywhere, Category = "Durability")
	int32 MaxDurability = 100;
};
