// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Components/Rpg_ItemComponent.h"

#include "Net/UnrealNetwork.h"
#include "Items/Fragments/ItemFragment.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "Items/Fragments/ConsumableFragment.h"
#include "Items/Fragments/StackableFragment.h"
#include "Items/Runtime/ItemRuntimeData.h"
#include "Items/Fragments/Rpg_FragmentTags.h"

void URpg_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemId);
	DOREPLIFETIME(ThisClass, RuntimeData);
}

int32 URpg_ItemComponent::GetMaxStackSize() const
{
	if (const URpg_ItemDefinition* Def = GetItemDefinition())
	{
		if (const FStackableFragment* Frag = Def->GetFragmentOfTypeWithTag<FStackableFragment>(FragmentTags::StackableFragment))
		{
			return FMath::Max(1, Frag->GetMaxStackSize());
		}
	}
	return 1;
}

int32 URpg_ItemComponent::GetCurrentStackCount() const
{
	if (const FStackableRuntimeData* Data = RuntimeData.FindConst<FStackableRuntimeData>(FragmentTags::StackableFragment))
	{
		return Data->CurrentStackCount;
	}
	return 1;
}

void URpg_ItemComponent::InitItemByDefinition(URpg_ItemDefinition* Definition)
{
	check(GetOwner() && GetOwner()->HasAuthority());

	ItemDefinition = Definition;
	ItemId   = Definition ? Definition->GetPrimaryAssetId() : FPrimaryAssetId();

	InitRuntimeFromDefinition(Definition);
}

void URpg_ItemComponent::InitItemById(FPrimaryAssetId Id)
{
	check(GetOwner() && GetOwner()->HasAuthority());

	ItemId = Id;
	ItemDefinition = nullptr;

	// Optional: sofort laden, damit Stack initialisiert werden kann
	if (ItemId.IsValid())
	{
		FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(ItemId);
		if (Path.IsValid())
		{
			if (UObject* Obj = Path.TryLoad())
			{
				ItemDefinition = Cast<URpg_ItemDefinition>(Obj);
			}
		}
	}
	InitRuntimeFromDefinition(ItemDefinition.Get());
}

void URpg_ItemComponent::InitItemBySoft(TSoftObjectPtr<URpg_ItemDefinition> Soft)
{
	check(GetOwner() && GetOwner()->HasAuthority());
	URpg_ItemDefinition* Def = Soft.IsValid() ? Soft.Get() : Soft.LoadSynchronous();
	InitItemByDefinition(Def);
}

void URpg_ItemComponent::OnRep_ItemId()
{
	ItemDefinition = nullptr;

	if (!ItemId.IsValid()) return;

	FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(ItemId);
	if (Path.IsValid())
	{
		if (UObject* Obj = Path.TryLoad()) // für kleine DataAssets ok; sonst async
		{
			ItemDefinition = Cast<URpg_ItemDefinition>(Obj);
			// Initialize/refresh runtime data on clients when definition arrives
			InitRuntimeFromDefinition(ItemDefinition.Get());
		}
	}
}

void URpg_ItemComponent::OnRep_RuntimeData()
{
	// UI/FX-Refresh (Widgets, Sounds etc.) could be triggered here if needed
}

void URpg_ItemComponent::InitRuntimeFromDefinition(const URpg_ItemDefinition* Def)
{
	if (Def)
	{
		// Initialize Stackable runtime data if definition has the fragment
		if (const FStackableFragment* Stack = Def->GetFragmentOfTypeWithTag<FStackableFragment>(FragmentTags::StackableFragment))
		{
			const int32 Max = FMath::Max(1, Stack->GetMaxStackSize());
			if (FStackableRuntimeData* StackData = RuntimeData.FindOrAddMutable<FStackableRuntimeData>(FragmentTags::StackableFragment))
			{
				if (StackData->CurrentStackCount <= 0)
				{
					StackData->CurrentStackCount = 1;
				}
				StackData->CurrentStackCount = FMath::Clamp(StackData->CurrentStackCount, 1, Max);
				RuntimeData.MarkDirty(FragmentTags::StackableFragment);
			}
		}
	}
}

bool URpg_ItemComponent::Consume(APawn* Instigator)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		// auf Clients nie State ändern
		return false;
	}
	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("Consume failed: ItemData is null"));
		return false;
	}

	const FConsumableFragment* Consumable = ItemDefinition->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment);
	if (!Consumable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Consume failed: No ConsumableFragment on item"));
		return false;
	}

	// Check runtime stack if required
	if (Consumable->bReduceStack)
	{
		int32 Max = GetMaxStackSize();
		FStackableRuntimeData* StackData = RuntimeData.FindMutable<FStackableRuntimeData>(FragmentTags::StackableFragment);
		int32 Current = StackData ? StackData->CurrentStackCount : 1;
		if (Current < Consumable->QuantityPerUse)
		{
			UE_LOG(LogTemp, Warning, TEXT("Consume failed: Not enough stack. Have %d, need %d"), Current, Consumable->QuantityPerUse);
			return false;
		}
		Current = FMath::Clamp(Current - Consumable->QuantityPerUse, 0, Max);
		if (!StackData)
		{
			StackData = RuntimeData.FindOrAddMutable<FStackableRuntimeData>(FragmentTags::StackableFragment);
		}
		StackData->CurrentStackCount = Current;
		RuntimeData.MarkDirty(FragmentTags::StackableFragment);
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
	if (!bEnabled || !ItemDefinition) return FInteractDisplayData();

	FInteractDisplayData Data;
	
	Data.ActionText = ItemDefinition->GetInteractionText();
	
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
	URpg_ContainerComponent* InventoryComponent = nullptr;

	if (Instigator)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: Instigator is valid"));
		
		if (AController* Controller = Instigator->GetController())
		{
			UE_LOG(LogTemp, Warning, TEXT("Interact: Controller is valid"));
			
			// Prefer Inventory on Controller
			InventoryComponent = Controller->FindComponentByClass<URpg_ContainerComponent>();
			if (InventoryComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("Interact: Found InventoryComponent on Controller"));
			}
			
			// Fallback to PlayerState (survives level changes)
			if (!InventoryComponent && Controller->PlayerState)
			{
				UE_LOG(LogTemp, Warning, TEXT("Interact: PlayerState is valid, searching for component..."));
				InventoryComponent = Controller->PlayerState->FindComponentByClass<URpg_ContainerComponent>();
				
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
			InventoryComponent = Instigator->FindComponentByClass<URpg_ContainerComponent>();
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
			InventoryComponent = OwnerActor->FindComponentByClass<URpg_ContainerComponent>();
			if (InventoryComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("Interact: Found InventoryComponent on Owner"));
			}
		}
	}

	if (InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: Successfully found InventoryComponent, attempting to consume"));
		if (const FConsumableFragment* ConsumableFragment = GetItemDefinition()->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment))
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

	// Level-platziert: Server übernimmt InitialDefinition einmalig
	if (GetOwner() && GetOwner()->HasAuthority() && InitialDefinition.IsValid())
	{
		URpg_ItemDefinition* Def = InitialDefinition.Get();
		if (!Def) Def = InitialDefinition.LoadSynchronous();
		InitItemByDefinition(Def);
	}
}

