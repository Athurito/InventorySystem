// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Components/Rpg_ItemComponent.h"

#include "Net/UnrealNetwork.h"
#include "Items/Fragments/ItemFragment.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "InventoryManagement/Components/Rpg_InventoryComponent.h"

void URpg_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemData);
	DOREPLIFETIME(ThisClass, CurrentStackCount);
	DOREPLIFETIME(ThisClass, MaxStackSize);
}

void URpg_ItemComponent::InitItemData(URpg_ItemDefinition* CopyOfItemData)
{
	ItemData = CopyOfItemData;

	// Initialize runtime stack from data asset once; do not mutate the asset
	if (ItemData)
	{
		if (const FStackableFragment* Stack = ItemData->GetFragmentOfTypeWithTag<FStackableFragment>(FragmentTags::StackableFragment))
		{
			MaxStackSize = FMath::Max(1, Stack->GetMaxStackSize());
			CurrentStackCount = FMath::Clamp(Stack->GetStackCount(), 0, MaxStackSize);
		}
		else
		{
			MaxStackSize = 1;
			CurrentStackCount = 1;
		}
	}
	else
	{
		MaxStackSize = 1;
		CurrentStackCount = 1;
	}
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

	// Check runtime stack if required
	if (Consumable->bReduceStack)
	{
		if (CurrentStackCount < Consumable->QuantityPerUse)
		{
			UE_LOG(LogTemp, Warning, TEXT("Consume failed: Not enough stack. Have %d, need %d"), CurrentStackCount, Consumable->QuantityPerUse);
			return false;
		}
		CurrentStackCount = FMath::Clamp(CurrentStackCount - Consumable->QuantityPerUse, 0, MaxStackSize);
	}

	// Durability not implemented yet in this module; log if requested
	if (Consumable->bReduceDurability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Consume: bReduceDurability is true but durability system not implemented. Skipping wear."));
	}

	// Helper to resolve an ASC from Instigator/Controller/PlayerState/Owner
	auto ResolveASC = [](APawn* InInstigator, AActor* InOwner) -> UAbilitySystemComponent*
	{
		if (InInstigator)
		{
			// Pawn implements ASI
			if (const IAbilitySystemInterface* ASIInst = Cast<IAbilitySystemInterface>(InInstigator))
			{
				if (UAbilitySystemComponent* C = ASIInst->GetAbilitySystemComponent()) return C;
			}
			// PlayerState usually holds ASC
			if (APlayerState* PS = InInstigator->GetPlayerState())
			{
				if (const IAbilitySystemInterface* ASIPS = Cast<IAbilitySystemInterface>(PS))
				{
					if (UAbilitySystemComponent* C = ASIPS->GetAbilitySystemComponent()) return C;
				}
			}
			// Controller may also implement
			if (AController* Cntr = InInstigator->GetController())
			{
				if (const IAbilitySystemInterface* ASIC = Cast<IAbilitySystemInterface>(Cntr))
				{
					if (UAbilitySystemComponent* C = ASIC->GetAbilitySystemComponent()) return C;
				}
			}
		}
		// Fallback to owner of item component
		if (InOwner)
		{
			if (const IAbilitySystemInterface* ASIOwner = Cast<IAbilitySystemInterface>(InOwner))
			{
				if (UAbilitySystemComponent* C = ASIOwner->GetAbilitySystemComponent()) return C;
			}
		}
		return nullptr;
	};

	// Apply gameplay effect if possible
	if (Consumable->ConsumableEffect)
	{
		UAbilitySystemComponent* ASC = ResolveASC(Instigator, GetOwner());
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
	URpg_InventoryComponent* InventoryComponent = nullptr;

	if (Instigator)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: Instigator is valid"));
		
		if (AController* Controller = Instigator->GetController())
		{
			UE_LOG(LogTemp, Warning, TEXT("Interact: Controller is valid"));
			
			// Prefer Inventory on Controller
			InventoryComponent = Controller->FindComponentByClass<URpg_InventoryComponent>();
			if (InventoryComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("Interact: Found InventoryComponent on Controller"));
			}
			
			// Fallback to PlayerState (survives level changes)
			if (!InventoryComponent && Controller->PlayerState)
			{
				UE_LOG(LogTemp, Warning, TEXT("Interact: PlayerState is valid, searching for component..."));
				InventoryComponent = Controller->PlayerState->FindComponentByClass<URpg_InventoryComponent>();
				
				if (InventoryComponent)
				{
					UE_LOG(LogTemp, Warning, TEXT("Interact: Found InventoryComponent on PlayerState!"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Interact: PlayerState exists but NO InventoryComponent found!"));
				}
			}
			else if (!Controller->PlayerState)
			{
				UE_LOG(LogTemp, Error, TEXT("Interact: PlayerState is NULL!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Interact: Controller is NULL!"));
		}
		
		// As last resort, try on Pawn itself
		if (!InventoryComponent)
		{
			InventoryComponent = Instigator->FindComponentByClass<URpg_InventoryComponent>();
			if (InventoryComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("Interact: Found InventoryComponent on Pawn"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Interact: Instigator is NULL!"));
	}
	
	// Final fallback: try owner of this component
	if (!InventoryComponent)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			InventoryComponent = OwnerActor->FindComponentByClass<URpg_InventoryComponent>();
			if (InventoryComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("Interact: Found InventoryComponent on Owner"));
			}
		}
	}

	if (InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: Successfully found InventoryComponent, attempting to consume"));
		if (const FConsumableFragment* ConsumableFragment = GetItemData()->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment))
		{
			InventoryComponent->TryConsumeItem(this, ConsumableFragment->QuantityPerUse);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Interact: NO InventoryComponent found anywhere!"));
	}
}

void URpg_ItemComponent::BeginPlay()
{
	Super::BeginPlay();
}

