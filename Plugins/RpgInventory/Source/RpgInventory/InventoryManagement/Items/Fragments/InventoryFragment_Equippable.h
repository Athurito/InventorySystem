// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "GameplayTagContainer.h"
#include "InventoryFragment_Equippable.generated.h"

class URpgEquipmentDefinition;

/**
 * Fragment to make an item equippable.
 */
UCLASS()
class RPGINVENTORY_API UInventoryFragment_Equippable : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	// The equipment definition that defines actors and abilities
	UPROPERTY(EditAnywhere, Category = "Equipment")
	TSubclassOf<URpgEquipmentDefinition> EquipmentDefinition;

	// Valid slots for this item
	UPROPERTY(EditAnywhere, Category = "Equipment")
	FGameplayTagContainer SupportedSlots;
};
