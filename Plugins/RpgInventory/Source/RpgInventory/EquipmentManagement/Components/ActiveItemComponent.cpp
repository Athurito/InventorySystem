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

	if (ActiveSlotIndex == EquipmentSlotIndex)
	{
		// Toggle off if same slot is pressed?
		ActiveSlotIndex = INDEX_NONE;
	}
	else
	{
		ActiveSlotIndex = EquipmentSlotIndex;
	}

	UpdateActiveVisuals();
	
	// Update abilities
	RemoveAbilities();
	if (ActiveSlotIndex != INDEX_NONE && InventoryManager)
	{
		int32 EquipContainer = InventoryManager->GetFirstContainerIndexByType(EInventorySlotType::Equipment);
		if (UInventoryItemInstance* Item = InventoryManager->GetItemInstanceInSlot(ActiveSlotIndex, EquipContainer))
		{
			GrantAbilities(Item);
		}
	}
}

void UActiveItemComponent::OnRep_ActiveSlotIndex(int32 OldIndex)
{
	UpdateActiveVisuals();
}

void UActiveItemComponent::UpdateActiveVisuals()
{
    if (EquipmentManager)
    {
        EquipmentManager->NotifyActiveSlotChanged(ActiveSlotIndex);
    }
}

void UActiveItemComponent::GrantAbilities(UInventoryItemInstance* Item)
{
	if (!Item) return;
	const UInventoryFragment_Equippable* EquipFrag = Item->FindFragmentByClass<UInventoryFragment_Equippable>();
	if (!EquipFrag) return;

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			for (auto AbilityClass : EquipFrag->GrantedAbilities)
			{
				if (AbilityClass)
				{
					GrantedAbilityHandles.Add(ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1)));
				}
			}
		}
	}
}

void UActiveItemComponent::RemoveAbilities()
{
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			for (auto& Handle : GrantedAbilityHandles)
			{
				if (Handle.IsValid())
				{
					ASC->ClearAbility(Handle);
				}
			}
		}
	}
	GrantedAbilityHandles.Empty();
}

void UActiveItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UActiveItemComponent, ActiveSlotIndex);
}
