// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/FastArray/SlotArray.h"

#include "InventoryManagement/Components/Rpg_ContainerComponent.h"

void FInvSlotArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	if (auto* CC = Cast<URpg_ContainerComponent>(OwnerComponent))
	{
		for (int32 Idx : RemovedIndices)
		{
			const FInv_Slot& S = Items[Idx];
			CC->BroadcastSlotChangedForOwner(this, S.SlotIndex, FGuid()); // InstanceId nun leer
		}
	}
}

void FInvSlotArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	if (auto* CC = Cast<URpg_ContainerComponent>(OwnerComponent))
	{
		for (int32 Idx : AddedIndices)
		{
			const FInv_Slot& S = Items[Idx];
			CC->BroadcastSlotChangedForOwner(this, S.SlotIndex, S.InstanceId);
		}
	}
}

void FInvSlotArray::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	if (auto* CC = Cast<URpg_ContainerComponent>(OwnerComponent))
	{
		for (int32 Idx : ChangedIndices)
		{
			const FInv_Slot& S = Items[Idx];
			CC->BroadcastSlotChangedForOwner(this, S.SlotIndex, S.InstanceId);
		}
	}
}
