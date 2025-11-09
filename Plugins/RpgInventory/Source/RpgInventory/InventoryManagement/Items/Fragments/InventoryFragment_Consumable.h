// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "Rpg_FragmentTags.h"
#include "GameplayTagContainer.h"

#include "InventoryFragment_Consumable.generated.h"

class UGameplayEffect;
class UGameplayAbility;
struct FItemRuntimeDataContainer;
class UInventoryItemDefinition;

UENUM(BlueprintType)
enum class EUseContext : uint8 { World, Inventory, Hotbar };

UENUM(BlueprintType)
enum class EUseAvailability : uint8 {
	WorldOnly,
	InventoryOnly,
	WorldOrInventory,
	PickupThenUseIfWorld
};

/**
 * Consumable rules and effect definition
 */
USTRUCT(BlueprintType)
struct FInventoryFragment_Consumable : public UInventoryItemFragment
{
	GENERATED_BODY()

	// Where can this item be used from?
	UPROPERTY(EditAnywhere, Category = "Consumable|Use")
	EUseAvailability UseAvailability = EUseAvailability::WorldOrInventory;

	// Optional: Event tag to trigger an already granted ability via GameplayEvent (Blueprint/editor-driven)
	UPROPERTY(EditAnywhere, Category = "Consumable|Ability")
	FGameplayTag UseEventTag;
	
	// Level used when applying the Gameplay Effect. Defaults to 1.0
	UPROPERTY(EditAnywhere, Category = "Consumable|Effect")
	float EffectLevel = 1.0f;

	// Whether consuming this item should reduce its stack
	UPROPERTY(EditAnywhere, Category = "Consumable|Costs")
	bool bReduceStack = true;

	// Stack quantity to consume (only if ReduceStack is true)
	UPROPERTY(EditAnywhere, Category = "Consumable|Costs", meta=(EditCondition="bReduceStack", ClampMin="1"))
	int32 QuantityPerUse = 1;

	// Whether consuming this item should apply durability wear
	UPROPERTY(EditAnywhere, Category = "Consumable|Costs")
	bool bReduceDurability = false;

	// Amount of durability wear applied on use (only if ReduceDurability is true)
	UPROPERTY(EditAnywhere, Category = "Consumable|Costs", meta=(EditCondition="bReduceDurability", ClampMin="0.0"))
	float WearPerUse = 0.f;

	// After picking up in world, automatically use (only relevant with PickupThenUseIfWorld policy)
	UPROPERTY(EditAnywhere, Category = "Consumable|Flow")
	bool bAutoUseAfterPickup = false;
	
	// Fragment-centric helper methods (C++)
	bool AllowsContext(EUseContext Ctx) const;
	bool PreflightCanUse(const FItemRuntimeDataContainer& Runtime, const UInventoryItemDefinition* Def) const;
	bool ReduceStackAfterUse(FItemRuntimeDataContainer& Runtime, const UInventoryItemDefinition* Def, int32 Uses) const;
};
