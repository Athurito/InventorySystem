// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayEffectTypes.h"
#include "InventoryAbilitySet.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct RPGINVENTORY_API FInventoryAbilitySetHandles
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY(Transient)
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	void Reset()
	{
		AbilitySpecHandles.Reset();
		GameplayEffectHandles.Reset();
	}

	bool IsEmpty() const
	{
		return AbilitySpecHandles.Num() == 0 && GameplayEffectHandles.Num() == 0;
	}
};

/**
 * Lyra-style (light) ability set: a bundle of abilities and gameplay effects.
 * This is intended to be applied/removed by equip/active systems.
 */
UCLASS(BlueprintType)
class RPGINVENTORY_API UInventoryAbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Abilities to grant when this set is applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AbilitySet")
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

	/** Gameplay effects to apply when this set is applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AbilitySet")
	TArray<TSubclassOf<UGameplayEffect>> GrantedGameplayEffects;

	/**
	 * Server-authoritative apply. Tracks handles so removal is deterministic.
	 * @param SourceObject Used as the ability source (optional).
	 */
	void ApplyToASC(UAbilitySystemComponent* ASC, UObject* SourceObject, FInventoryAbilitySetHandles& OutHandles) const;

	/**
	 * Server-authoritative remove by handles.
	 */
	static void RemoveFromASC(UAbilitySystemComponent* ASC, FInventoryAbilitySetHandles& Handles);
};
