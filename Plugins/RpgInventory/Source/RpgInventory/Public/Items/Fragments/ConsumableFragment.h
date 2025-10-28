// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "Rpg_FragmentTags.h"

#include "ConsumableFragment.generated.h"

class UGameplayEffect;
class UGameplayAbility;
struct FItemRuntimeDataContainer;
class URpg_ItemDefinition;

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
struct FConsumableFragment : public FItemFragment
{
	GENERATED_BODY()
	FConsumableFragment()
	{
		SetFragmentTag(FragmentTags::ConsumableFragment);
	}

	// Where can this item be used from?
	UPROPERTY(EditAnywhere, Category = "Consumable|Use")
	EUseAvailability UseAvailability = EUseAvailability::WorldOrInventory;

	// Ability to activate when using this item (defined in editor on the fragment)
	UPROPERTY(EditAnywhere, Category = "Consumable|Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	// Optional cooldown GameplayEffect to apply when used (can also be configured inside the Ability)
	UPROPERTY(EditAnywhere, Category = "Consumable|Ability")
	TSubclassOf<UGameplayEffect> CooldownEffect;

	// Gameplay Effect to apply when consumed (legacy/fallback if no Ability is set)
	UPROPERTY(EditAnywhere, Category = "Consumable|Effect")
	TSubclassOf<UGameplayEffect> ConsumableEffect;

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
	bool PreflightCanUse(const FItemRuntimeDataContainer& Runtime, const URpg_ItemDefinition* Def) const;
	bool ReduceStackAfterUse(FItemRuntimeDataContainer& Runtime, const URpg_ItemDefinition* Def, int32 Uses) const;
};
