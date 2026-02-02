// Fill out your copyright notice in the Description page of Project Settings.


#include "ActiveItemComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_WeaponConfig.h"

UActiveItemComponent::UActiveItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UActiveItemComponent::InitializeFromInventory(UInventoryManagerComponent* InInventoryManager)
{
	InventoryManager = InInventoryManager;
	HotbarContainerIndex = INDEX_NONE;

	if (InventoryManager)
	{
		HotbarContainerIndex = InventoryManager->GetFirstContainerIndexByType(EInventorySlotType::Hotbar);
	}

	// If we already have an active slot (e.g. replicated), ensure grants are in sync.
	if (ActiveHotbarSlotIndex != INDEX_NONE)
	{
		RemoveActiveGrants();
		if (InventoryManager && HotbarContainerIndex != INDEX_NONE)
		{
			if (UInventoryItemInstance* Item = InventoryManager->GetItemInstanceInSlot(ActiveHotbarSlotIndex, HotbarContainerIndex))
			{
				ApplyActiveGrants(Item);
			}
		}
	}
}

void UActiveItemComponent::BeginPlay()
{
	Super::BeginPlay();
	// Wiring happens via `URpgInventoryWiringComponent`.
}

void UActiveItemComponent::SetActiveSlot(int32 HotbarSlotIndex)
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		return;
	}

	// Safety: if not wired yet, try a one-shot resolve (no timers).
	if (!InventoryManager)
	{
		if (const APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			if (APlayerState* PS = Pawn->GetPlayerState())
			{
				InventoryManager = PS->FindComponentByClass<UInventoryManagerComponent>();
			}
		}
		if (InventoryManager && HotbarContainerIndex == INDEX_NONE)
		{
			HotbarContainerIndex = InventoryManager->GetFirstContainerIndexByType(EInventorySlotType::Hotbar);
		}
	}

	const int32 OldIndex = ActiveHotbarSlotIndex;

	if (ActiveHotbarSlotIndex == HotbarSlotIndex)
	{
		// Toggle off if same slot is pressed?
		ActiveHotbarSlotIndex = INDEX_NONE;
	}
	else
	{
		ActiveHotbarSlotIndex = HotbarSlotIndex;
	}

	BroadcastActiveSlotChanged(OldIndex);
	
	// Update abilities/effects granted while active
	RemoveActiveGrants();
	if (ActiveHotbarSlotIndex != INDEX_NONE && InventoryManager)
	{
		if (HotbarContainerIndex == INDEX_NONE)
		{
			HotbarContainerIndex = InventoryManager->GetFirstContainerIndexByType(EInventorySlotType::Hotbar);
		}
		if (HotbarContainerIndex != INDEX_NONE)
		{
			if (UInventoryItemInstance* Item = InventoryManager->GetItemInstanceInSlot(ActiveHotbarSlotIndex, HotbarContainerIndex))
			{
				ApplyActiveGrants(Item);
			}
		}
	}
}

void UActiveItemComponent::OnRep_ActiveHotbarSlotIndex(int32 OldIndex)
{
	BroadcastActiveSlotChanged(OldIndex);
}

void UActiveItemComponent::BroadcastActiveSlotChanged(int32 OldIndex)
{
	OnActiveHotbarSlotChanged.Broadcast(ActiveHotbarSlotIndex, OldIndex);
}

void UActiveItemComponent::ApplyActiveGrants(UInventoryItemInstance* Item)
{
	if (!Item) return;

	// ASC lives on PlayerState (Lyra-style)
	UAbilitySystemComponent* ASC = nullptr;
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerState* PS = Pawn->GetPlayerState())
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
			{
				ASC = ASI->GetAbilitySystemComponent();
			}
		}
	}
	if (!ASC)
	{
		return;
	}

	// 1) Apply active ability sets (preferred)
	if (const UInventoryFragment_WeaponConfig* WeaponFrag = Item->FindFragmentByClass<UInventoryFragment_WeaponConfig>())
	{
		for (const UInventoryAbilitySet* Set : WeaponFrag->ActiveAbilitySets)
		{
			if (Set)
			{
				Set->ApplyToASC(ASC, Item, ActiveAbilitySetHandles);
			}
		}
	}
}

void UActiveItemComponent::RemoveActiveGrants()
{
	UAbilitySystemComponent* ASC = nullptr;
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerState* PS = Pawn->GetPlayerState())
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
			{
				ASC = ASI->GetAbilitySystemComponent();
			}
		}
	}
	if (ASC)
	{
		UInventoryAbilitySet::RemoveFromASC(ASC, ActiveAbilitySetHandles);
	}
}

void UActiveItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UActiveItemComponent, ActiveHotbarSlotIndex);
}
