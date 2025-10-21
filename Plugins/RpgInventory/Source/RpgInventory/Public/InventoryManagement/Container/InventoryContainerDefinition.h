// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "InventoryContainerDefinition.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EInventorySlotType : uint8 { Generic, Consumable, Quest, Equipment };

UCLASS()
class RPGINVENTORY_API UInventoryContainerDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FText  DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EInventorySlotType Type = EInventorySlotType::Generic;

	// Grid
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) int32 Rows = 5;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) int32 Cols = 6;

	// Welche Items sind erlaubt? (optional – sonst alles)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FGameplayTagQuery AllowedItems;

	// Optional für UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) UTexture2D* TabIcon = nullptr;
};
