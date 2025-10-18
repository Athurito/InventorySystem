// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemFragment.h"
#include "Items/Fragments/Rpg_FragmentTags.h"
#include "ConsumableFragment.generated.h"

class UGameplayEffect;
/**
 * 
 */
USTRUCT(BlueprintType)
struct FConsumableFragment : public FItemFragment
{
	GENERATED_BODY()
	FConsumableFragment()
	{
		SetFragmentTag(FragmentTags::ConsumableFragment);
	}

	// Gameplay Effect to apply when consumed
	UPROPERTY(EditAnywhere, Category = "Consumable")
	TSubclassOf<UGameplayEffect> ConsumableEffect;

	// Level used when applying the Gameplay Effect. Defaults to 1.0
	UPROPERTY(EditAnywhere, Category = "Consumable")
	float EffectLevel = 1.0f;

	// Whether consuming this item should reduce its stack
	UPROPERTY(EditAnywhere, Category = "Consumable")
	bool bReduceStack = true;

	// Stack quantity to consume (only if ReduceStack is true)
	UPROPERTY(EditAnywhere, Category = "Consumable", meta=(EditCondition="bReduceStack", ClampMin="1"))
	int32 QuantityPerUse = 1;

	// Whether consuming this item should apply durability wear
	UPROPERTY(EditAnywhere, Category = "Consumable")
	bool bReduceDurability = false;

	// Amount of durability wear applied on use (only if ReduceDurability is true)
	UPROPERTY(EditAnywhere, Category = "Consumable", meta=(EditCondition="bReduceDurability", ClampMin="0.0"))
	float WearPerUse = 0.f;
};
