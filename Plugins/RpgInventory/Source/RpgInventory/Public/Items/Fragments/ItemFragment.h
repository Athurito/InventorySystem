// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Rpg_FragmentTags.h"
#include "UObject/Object.h"
#include "GameplayEffect.h"
#include "ItemFragment.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FItemFragment
{
	GENERATED_BODY()

	FItemFragment() {  }
	FItemFragment(const FItemFragment&) = default;
	FItemFragment& operator=(const FItemFragment&) = default;
	FItemFragment(FItemFragment&&) = default;
	FItemFragment& operator=(FItemFragment&&) = default;
	virtual ~FItemFragment() {}


	FGameplayTag GetFragmentTag() const { return FragmentTag; }
	void SetFragmentTag(const FGameplayTag Tag) { FragmentTag = Tag; }
	virtual void Manifest() {}
	
private:
	
	FGameplayTag FragmentTag = FGameplayTag::EmptyTag;
};

USTRUCT(BlueprintType)
struct FStackableFragment : public FItemFragment
{
	GENERATED_BODY()

	FStackableFragment()
	{
		SetFragmentTag(FragmentTags::StackableFragment);
	}
	int32 GetMaxStackSize() const { return MaxStackSize; }
	int32 GetStackCount() const { return StackCount; }
	void SetStackCount(const int32 Count) { StackCount = Count; }

private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 MaxStackSize{1};
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 StackCount{1};
};

// Defines how an item can be consumed (effects to apply and how to deplete it)
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
	TSubclassOf<class UGameplayEffect> ConsumableEffect;

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