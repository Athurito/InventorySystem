// Fill out your copyright notice in the Description page of Project Settings.


#include "ActiveItemComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "EquipmentManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Equippable.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_WeaponConfig.h"

UActiveItemComponent::UActiveItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UActiveItemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerState* PS = OwningPawn->GetPlayerState())
		{
			InventoryManager = PS->FindComponentByClass<UInventoryManagerComponent>();
		}
	}
	
	if (!InventoryManager)
	{
		InventoryManager = GetOwner()->FindComponentByClass<UInventoryManagerComponent>();
	}

	EquipmentManager = GetOwner()->FindComponentByClass<UEquipmentManagerComponent>();
}

void UActiveItemComponent::SetActiveSlot(int32 EquipmentSlotIndex)
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		return;
	}

	const int32 OldIndex = ActiveSlotIndex;

	if (ActiveSlotIndex == EquipmentSlotIndex)
	{
		// Toggle off if same slot is pressed?
		ActiveSlotIndex = INDEX_NONE;
	}
	else
	{
		ActiveSlotIndex = EquipmentSlotIndex;
	}

	BroadcastActiveSlotChanged(OldIndex);

	UpdateActiveVisuals();
	
	// Update abilities/effects granted while active
	RemoveActiveGrants();
	if (ActiveSlotIndex != INDEX_NONE && InventoryManager)
	{
		int32 EquipContainer = InventoryManager->GetFirstContainerIndexByType(EInventorySlotType::Equipment);
		if (UInventoryItemInstance* Item = InventoryManager->GetItemInstanceInSlot(ActiveSlotIndex, EquipContainer))
		{
			ApplyActiveGrants(Item);
		}
	}
}

void UActiveItemComponent::OnRep_ActiveSlotIndex(int32 OldIndex)
{
	BroadcastActiveSlotChanged(OldIndex);
	UpdateActiveVisuals();
}

void UActiveItemComponent::UpdateActiveVisuals()
{
    if (EquipmentManager)
    {
        EquipmentManager->NotifyActiveSlotChanged(ActiveSlotIndex);
    }
}

void UActiveItemComponent::BroadcastActiveSlotChanged(int32 OldIndex)
{
	OnActiveEquipmentSlotChanged.Broadcast(ActiveSlotIndex, OldIndex);
}

void UActiveItemComponent::ApplyActiveGrants(UInventoryItemInstance* Item)
{
	if (!Item) return;
	const UInventoryFragment_Equippable* EquipFrag = Item->FindFragmentByClass<UInventoryFragment_Equippable>();
	if (!EquipFrag) return;

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

	// 2) Legacy direct grants (optional)
	for (auto AbilityClass : EquipFrag->GrantedAbilities)
	{
		if (AbilityClass)
		{
			LegacyGrantedAbilityHandles.Add(ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1)));
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

		for (auto& Handle : LegacyGrantedAbilityHandles)
		{
			if (Handle.IsValid())
			{
				ASC->ClearAbility(Handle);
			}
		}
	}
	LegacyGrantedAbilityHandles.Empty();
}

void UActiveItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UActiveItemComponent, ActiveSlotIndex);
}
