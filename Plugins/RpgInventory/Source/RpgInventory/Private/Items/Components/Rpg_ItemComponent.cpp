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
#include "InventoryManagement/Utils/InventoryStatics.h"
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
	
	InventoryComponent = UInventoryStatics::ResolveInventoryFromInstigator(Instigator);
	
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Interact: NO InventoryComponent found anywhere!"));
		return;
	}
	
	const URpg_ItemDefinition* Def = GetItemDefinition();
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: No ItemDefinition, aborting"));
		return;
	}

	// 1) If item is consumable, consume it via container rules
	if (const FConsumableFragment* ConsumableFragment = Def->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment))
	{
		InventoryComponent->TryConsumeItem(this, FMath::Max(1, ConsumableFragment->QuantityPerUse));
		return;
	}

	// 2) Otherwise: attempt to pick up into an appropriate container
	const FGameplayTag ItemType = Def->GetItemType();
	int32 TargetContainerIdx = INDEX_NONE;
	// Find first container that allows this item type
	for (int32 i = 0; i < InventoryComponent->Containers.Num(); ++i)
	{
		if (InventoryComponent->Containers[i].IsItemAllowed(ItemType))
		{
			TargetContainerIdx = i;
			break;
		}
	}
	if (TargetContainerIdx == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: No accepting container for item type %s"), *ItemType.ToString());
		return;
	}

	// Determine quantity to pick up
	int32 QuantityToAdd = 1;
	if (Def->GetFragmentOfTypeWithTag<FStackableFragment>(FragmentTags::StackableFragment))
	{
		QuantityToAdd = FMath::Max(1, GetCurrentStackCount());
	}

	int32 OutAdded = 0; FGuid OutInstanceId;
	const bool bRequested = InventoryComponent->AddItemToContainer(TargetContainerIdx, this, QuantityToAdd, OutAdded, OutInstanceId);

	// If we are on the server, reduce or destroy the world item based on how much was actually added
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (OutAdded > 0)
		{
			if (FStackableRuntimeData* StackData = RuntimeData.FindMutable<FStackableRuntimeData>(FragmentTags::StackableFragment))
			{
				StackData->CurrentStackCount = FMath::Max(0, StackData->CurrentStackCount - OutAdded);
				RuntimeData.MarkDirty(FragmentTags::StackableFragment);
				if (StackData->CurrentStackCount <= 0)
				{
					if (AActor* OwnerActor = GetOwner())
					{
						OwnerActor->Destroy();
					}
				}
			}
			else
			{
				// Non-stackable or no stack runtime data: if anything was added, just destroy the actor
				if (Def->GetFragmentOfTypeWithTag<FStackableFragment>(FragmentTags::StackableFragment) == nullptr)
				{
					if (AActor* OwnerActor = GetOwner())
					{
						OwnerActor->Destroy();
					}
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Interact: AddItemToContainer added nothing (full or disallowed)"));
		}
	}
	else
	{
		// On clients we just requested via RPC; the server will replicate inventory and possibly destroy the world item.
		UE_LOG(LogTemp, Verbose, TEXT("Interact: Pickup requested (client), waiting for replication"));
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

