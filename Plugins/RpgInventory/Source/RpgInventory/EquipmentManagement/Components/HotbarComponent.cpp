// Fill out your copyright notice in the Description page of Project Settings.


#include "HotbarComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"

UHotbarComponent::UHotbarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHotbarComponent::BeginPlay()
{
	Super::BeginPlay();

	// Falls die Komponente am Character hängt, suchen wir den InventoryManager am PlayerState
	if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerState* PS = OwningPawn->GetPlayerState())
		{
			InventoryManager = PS->FindComponentByClass<UInventoryManagerComponent>();
		}
	}

	// Fallback: Falls die Komponente direkt am PlayerState hängen (empfohlen)
	if (!InventoryManager)
	{
		InventoryManager = GetOwner()->FindComponentByClass<UInventoryManagerComponent>();
	}

	if (InventoryManager)
	{
		HotbarContainerIndex = InventoryManager->GetFirstContainerIndexByType(EInventorySlotType::Hotbar);
	}
}

void UHotbarComponent::UseHotbarSlot(int32 SlotIndex)
{
	if (!InventoryManager || HotbarContainerIndex == INDEX_NONE) return;
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ServerUseHotbarSlot(SlotIndex);
		return;
	}

	InventoryManager->UseItem(HotbarContainerIndex, SlotIndex);
	UE_LOG(LogTemp, Log, TEXT("Using item from Hotbar slot %d"), SlotIndex);
}

void UHotbarComponent::ServerUseHotbarSlot_Implementation(int32 SlotIndex)
{
	UseHotbarSlot(SlotIndex);
}

