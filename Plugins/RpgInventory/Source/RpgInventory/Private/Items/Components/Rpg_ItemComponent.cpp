// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Components/Rpg_ItemComponent.h"

#include "Net/UnrealNetwork.h"
#include "Items/Fragments/ItemFragment.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

void URpg_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemData);
}

void URpg_ItemComponent::InitItemData(UItemData* CopyOfItemData)
{
	ItemData = CopyOfItemData;
}

bool URpg_ItemComponent::Consume(APawn* Instigator)
{
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Consume failed: ItemData is null"));
		return false;
	}

	const FConsumableFragment* Consumable = ItemData->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment);
	if (!Consumable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Consume failed: No ConsumableFragment on item"));
		return false;
	}

	// Check stack if required
	if (Consumable->bReduceStack)
	{
		FStackableFragment* StackFrag = ItemData->GetFragmentOfTypeMutable<FStackableFragment>();
		if (!StackFrag)
		{
			UE_LOG(LogTemp, Warning, TEXT("Consume failed: bReduceStack true but no StackableFragment present"));
			return false;
		}
		if (StackFrag->GetStackCount() < Consumable->QuantityPerUse)
		{
			UE_LOG(LogTemp, Warning, TEXT("Consume failed: Not enough stack. Have %d, need %d"), StackFrag->GetStackCount(), Consumable->QuantityPerUse);
			return false;
		}
		StackFrag->SetStackCount(StackFrag->GetStackCount() - Consumable->QuantityPerUse);
	}

	// Durability not implemented yet in this module; log if requested
	if (Consumable->bReduceDurability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Consume: bReduceDurability is true but durability system not implemented. Skipping wear."));
	}

	// Apply gameplay effect if possible
	if (Consumable->ConsumableEffect)
	{
		UAbilitySystemComponent* ASC = nullptr;
		if (Instigator)
		{
			if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Instigator))
			{
				ASC = ASI->GetAbilitySystemComponent();
			}
			else if (AActor* InstigatorActor = Cast<AActor>(Instigator))
			{
				if (const IAbilitySystemInterface* ASIActor = Cast<IAbilitySystemInterface>(InstigatorActor))
				{
					ASC = ASIActor->GetAbilitySystemComponent();
				}
			}
		}

		if (!ASC)
		{
			// try owner
			if (AActor* OwnerActor = GetOwner())
			{
				if (const IAbilitySystemInterface* ASIOwner = Cast<IAbilitySystemInterface>(OwnerActor))
				{
					ASC = ASIOwner->GetAbilitySystemComponent();
				}
			}
		}

		if (ASC)
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Consumable->ConsumableEffect, Consumable->EffectLevel, Ctx);
			if (Spec.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Consume: No AbilitySystemComponent found to apply effect. Proceeding without effect."));
		}
	}

	return true;
}


FInteractDisplayData URpg_ItemComponent::GetDisplayData_Implementation() const
{
	if (!bEnabled || !ItemData) return FInteractDisplayData();

	FInteractDisplayData Data;
	
	Data.ActionText = ItemData->GetInteractionText();
	
	return Data;
}

bool URpg_ItemComponent::CanInteract_Implementation(APawn* Instigator) const
{
	if (!bEnabled || !GetOwner()) return false;
	if (!Instigator) return false;

	const float Dist = FVector::Dist(Instigator->GetActorLocation(), GetOwner()->GetActorLocation());
	return Dist <= MaxUseDistance;
}

void URpg_ItemComponent::Interact_Implementation(APawn* Instigator)
{
	IInteractable::Interact_Implementation(Instigator);
}

void URpg_ItemComponent::BeginPlay()
{
	Super::BeginPlay();
}

