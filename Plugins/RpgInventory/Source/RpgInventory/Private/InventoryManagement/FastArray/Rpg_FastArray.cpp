// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagement/FastArray/Rpg_FastArray.h"

#include "InventoryManagement/Components/Rpg_ContainerComponent.h"


void FInvContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	const TObjectPtr<URpg_ContainerComponent> InventoryComponent = Cast<URpg_ContainerComponent>(OwnerComponent);

	if (!InventoryComponent)
	{
		return;
	}

	for (int32 const Index : RemovedIndices)
	{
		InventoryComponent->OnItemRemoved.Broadcast(Entries[Index]);
	}
}

void FInvContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	const TObjectPtr<URpg_ContainerComponent> InventoryComponent = Cast<URpg_ContainerComponent>(OwnerComponent);

	if (!InventoryComponent)
	{
		return;
	}

	for (int32 const Index : AddedIndices)
	{
		InventoryComponent->OnItemAdded.Broadcast(Entries[Index]);
	}
}
