// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemComponent.h"

#include "Net/UnrealNetwork.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Stackable.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Pickup.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_SetStats.h"
#include "RpgInventory/InventoryManagement/Utils/InventoryStatics.h"

void UInventoryItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemId);
}

int32 UInventoryItemComponent::GetMaxStackSize() const
{
	if (const UInventoryItemDefinition* Def = GetItemDefinition())
	{
		if (UInventoryFragment_Stackable* StackableFragment = Cast<UInventoryFragment_Stackable>(Def->FindFragmentByClass(UInventoryFragment_Stackable::StaticClass())))
		{
			return StackableFragment->GetMaxStackSize();
		}
	}
	return 1;
}


void UInventoryItemComponent::InitItemByDefinition(UInventoryItemDefinition* Definition)
{
	check(GetOwner() && GetOwner()->HasAuthority());

	ItemDefinition = Definition;
	ItemId   = Definition ? Definition->GetPrimaryAssetId() : FPrimaryAssetId();

	// InitRuntimeFromDefinition(Definition);
}

void UInventoryItemComponent::InitItemById(FPrimaryAssetId Id)
{
	check(GetOwner() && GetOwner()->HasAuthority());
	
	if (UInventoryItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(Id))
	{
		InitItemByDefinition(Def);
	}
}

void UInventoryItemComponent::InitItemBySoft(TSoftObjectPtr<UInventoryItemDefinition> Soft)
{
	check(GetOwner() && GetOwner()->HasAuthority());
	UInventoryItemDefinition* Def = Soft.IsValid() ? Soft.Get() : Soft.LoadSynchronous();
	InitItemByDefinition(Def);
}

void UInventoryItemComponent::OnRep_ItemId()
{
	if (!ItemId.IsValid())
	{
		ItemDefinition = nullptr;
		return;
	}

	FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(ItemId);
	if (Path.IsValid())
	{
		if (UObject* Obj = Path.TryLoad())
		{
			ItemDefinition = Cast<UInventoryItemDefinition>(Obj);
		}
	}
}

FInteractDisplayData UInventoryItemComponent::GetDisplayData_Implementation() const
{
	FInteractDisplayData Data;
	if (!bEnabled) return Data;

	const UInventoryItemDefinition* Def = GetItemDefinition();
	if (!Def) return Data;

	if (const UInventoryFragment_Pickup* PickupFragment = Cast<UInventoryFragment_Pickup>(Def->FindFragmentByClass(UInventoryFragment_Pickup::StaticClass())))
	{
		Data.ActionText = PickupFragment->GetInteractionText();
	}
	else
	{
		Data.ActionText = FText::Format(NSLOCTEXT("Inventory", "PickupAction", "Pick up {0}"), FText::FromName(Def->GetFName()));
	}

	return Data;
}

bool UInventoryItemComponent::CanInteract_Implementation(APawn* Instigator) const
{
	if (!bEnabled || !GetOwner()) return false;
	if (!Instigator) return false;

	const float Dist = FVector::Dist(Instigator->GetActorLocation(), GetOwner()->GetActorLocation());
	return Dist <= MaxUseDistance;
}

void UInventoryItemComponent::Interact_Implementation(APawn* Instigator)
{
	UInventoryManagerComponent* InventoryComponent = nullptr;
	
	InventoryComponent = UInventoryStatics::ResolveInventoryFromInstigator(Instigator);
	
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Interact: NO InventoryComponent found anywhere!"));
		return;
	}
	
	const UInventoryItemDefinition* Def = GetItemDefinition();
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: No ItemDefinition, aborting"));
		return;
	}

	// // 1) If item is consumable, respect policy
	// if (const FConsumableFragment* Cons = Def->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment))
	// {
	// 	switch (Cons->UseAvailability)
	// 	{
	// 		case EUseAvailability::WorldOnly:
	// 		case EUseAvailability::WorldOrInventory:
	// 		{
	// 			// InventoryComponent->TryUseWorldItem(this, FMath::Max(1, Cons->QuantityPerUse));
	// 			return;
	// 		}
	// 		case EUseAvailability::InventoryOnly:
	// 		{
	// 			// Not usable directly in world; fall through to pickup
	// 			break;
	// 		}
	// 		case EUseAvailability::PickupThenUseIfWorld:
	// 		{
	// 			// We will pick up below; after successful pickup we may auto use
	// 			break;
	// 		}
	// 	}
	// }

	// 2) Otherwise: attempt to pick up into an appropriate container
	const FGameplayTag ItemType = Def->GetItemType();
	int32 TargetContainerIdx = 0; // Default to first container for now
	
	// Determine quantity to pick up
	int32 QuantityToAdd = 1;

	// In Lyra, initial stats (like stack count) are often defined in the ItemDefinition's fragments.
	// If there's a Fragment_SetStats that defines a stack count, we can use that as the default.
	if (const UInventoryFragment_SetStats* SetStatsFragment = Cast<UInventoryFragment_SetStats>(Def->FindFragmentByClass(UInventoryFragment_SetStats::StaticClass())))
	{
		int32 FragmentQuantity = SetStatsFragment->GetStatTagStackCount(FragmentTags::StackableFragment);
		if (FragmentQuantity > 0)
		{
			QuantityToAdd = FragmentQuantity;
		}
	}
	
	// Try to find a free slot or stackable slot
	int32 TargetSlotIndex = INDEX_NONE;
	const int32 NumSlots = InventoryComponent->GetNumSlots(TargetContainerIdx);
	for (int32 i = 0; i < NumSlots; ++i)
	{
		UInventoryItemInstance* Existing = InventoryComponent->GetItemInstanceInSlot(i, TargetContainerIdx);
		if (!Existing)
		{
			TargetSlotIndex = i;
			break;
		}
		// Optional: Implement stack merging logic here if desired
	}

	if (TargetSlotIndex != INDEX_NONE)
	{
		InventoryComponent->AddItemDefinition(const_cast<UInventoryItemDefinition*>(Def), TargetSlotIndex, TargetContainerIdx, QuantityToAdd);
		
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			GetOwner()->Destroy();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: No free slot in container %d"), TargetContainerIdx);
	}
}

void UInventoryItemComponent::BeginPlay()
{
	Super::BeginPlay();

	// Level-platziert: Server übernimmt InitialDefinition einmalig
	if (GetOwner() && GetOwner()->HasAuthority() && ItemDefinition.IsValid())
	{
		UInventoryItemDefinition* Def = ItemDefinition.Get();
		if (!Def) Def = ItemDefinition.LoadSynchronous();
		InitItemByDefinition(Def);
	}
}

