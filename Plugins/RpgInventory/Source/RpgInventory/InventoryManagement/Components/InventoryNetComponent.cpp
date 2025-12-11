// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryNetComponent.h"

#include "InventoryManagerComponent.h"


void UInventoryNetComponent::Server_HandleDrop_Implementation(UInventoryManagerComponent* SourceManager,
                                                              int32 SourceContainerIndex, int32 SourceSlotIndex, UInventoryManagerComponent* TargetManager,
                                                              int32 TargetContainerIndex, int32 TargetSlotIndex, int32 DragQuantity, FGameplayTag OperationType)
{
	if (!TargetManager)
	{
		return;
	}

	// reine Logik-Funktion im Manager, kein RPC
	TargetManager->HandleDrop_Internal(
		SourceManager,
		SourceContainerIndex,
		SourceSlotIndex,
		TargetContainerIndex,
		TargetSlotIndex,
		DragQuantity,
		OperationType);
}
