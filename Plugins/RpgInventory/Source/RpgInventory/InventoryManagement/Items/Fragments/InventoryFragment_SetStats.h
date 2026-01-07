// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "GameplayTagContainer.h"
#include "InventoryFragment_SetStats.generated.h"

/**
 * Fragment to set initial stats on an item instance
 */
UCLASS()
class UInventoryFragment_SetStats : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const override;

	int32 GetStatTagStackCount(FGameplayTag Tag) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	TMap<FGameplayTag, int32> InitialStatTags;
};
