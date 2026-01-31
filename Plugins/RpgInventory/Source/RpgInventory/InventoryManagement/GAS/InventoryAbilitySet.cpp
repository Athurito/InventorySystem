// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryAbilitySet.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"

void UInventoryAbilitySet::ApplyToASC(UAbilitySystemComponent* ASC, UObject* SourceObject, FInventoryAbilitySetHandles& OutHandles) const
{
	if (!ASC)
	{
		return;
	}
	if (!ASC->GetOwner() || !ASC->GetOwner()->HasAuthority())
	{
		return;
	}

	// Abilities
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : GrantedAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(AbilityClass, 1);
		Spec.SourceObject = SourceObject;
		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		OutHandles.AbilitySpecHandles.Add(Handle);
	}

	// Gameplay Effects
	for (const TSubclassOf<UGameplayEffect>& EffectClass : GrantedGameplayEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(SourceObject);
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
		if (Spec.IsValid())
		{
			const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			OutHandles.GameplayEffectHandles.Add(Handle);
		}
	}
}

void UInventoryAbilitySet::RemoveFromASC(UAbilitySystemComponent* ASC, FInventoryAbilitySetHandles& Handles)
{
	if (!ASC)
	{
		Handles.Reset();
		return;
	}
	if (!ASC->GetOwner() || !ASC->GetOwner()->HasAuthority())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : Handles.AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : Handles.GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	Handles.Reset();
}
