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
enum class EInventorySlotType : uint8 { Generic, Consumable, Quest, Equipment, Hotbar };

USTRUCT(BlueprintType)
struct FInventorySlotDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagQuery RequiredTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> BackgroundIcon;
};

UCLASS()
class RPGINVENTORY_API UInventoryContainerDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FText  DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EInventorySlotType Type = EInventorySlotType::Generic;

	// Grid
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) int32 TotalSlots = 5;

	// Welche Items sind erlaubt? (optional – sonst alles)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FGameplayTagQuery AllowedItems;

	// Spezifische Einstellungen pro Slot
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FInventorySlotDefinition> SlotDefinitions;

	// Optional für UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) UTexture2D* TabIcon = nullptr;
};
