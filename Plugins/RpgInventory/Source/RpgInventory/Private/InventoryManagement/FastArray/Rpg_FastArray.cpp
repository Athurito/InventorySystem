// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagement/FastArray/Rpg_FastArray.h"

#include "InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "Items/Rpg_ItemDefinition.h"


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


bool FInvContainer::IsItemAllowed(const FGameplayTag& ItemTag) const
{
	// Empty query means allow everything (e.g., generic storage)
 if (AllowedItems.IsEmpty())
	{
		return true;
	}
	FGameplayTagContainer Container;
	if (ItemTag.IsValid())
	{
		Container.AddTag(ItemTag);
	}
	return AllowedItems.Matches(Container);
}

int32 FInvContainer::FindIndexByInstance(const FGuid& InstanceId) const
{
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		if (Entries[i].GetInstanceId() == InstanceId)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 FInvContainer::AddOrStack(const FPrimaryAssetId& ItemId, const FGameplayTag& ItemType, int32 MaxStack, int32 Quantity, FGuid& OutInstanceId, int32& OutAdded)
{
	OutAdded = 0;
	if (Quantity <= 0 || MaxStack <= 0)
	{
		return INDEX_NONE;
	}
	if (!IsItemAllowed(ItemType))
	{
		return INDEX_NONE;
	}
	// First try to top up existing stacks of same item
	for (int32 i = 0; i < Entries.Num() && Quantity > 0; ++i)
	{
		FInv_InventoryEntry& E = Entries[i];
		if (E.GetItemId() != ItemId)
		{
			continue;
		}
		const int32 Free = MaxStack - E.GetStack();
		if (Free <= 0) continue;
		const int32 ToAdd = FMath::Min(Free, Quantity);
		E.SetStack(E.GetStack() + ToAdd);
		MarkItemDirty(E);
		Quantity -= ToAdd;
		OutAdded += ToAdd;
		OutInstanceId = E.GetInstanceId();
	}
	// Then create new stacks while quantity remains
	int32 LastIndex = INDEX_NONE;
	while (Quantity > 0)
	{
		FInv_InventoryEntry NewE;
		NewE.SetItemId(ItemId);
		NewE.SetItemType(ItemType);
		NewE.SetInstanceId(FGuid::NewGuid());
		const int32 ThisStack = FMath::Min(MaxStack, Quantity);
		NewE.SetStack(ThisStack);
		LastIndex = Entries.Add(NewE);
		MarkItemDirty(Entries[LastIndex]);
		Quantity -= ThisStack;
		OutAdded += ThisStack;
		OutInstanceId = NewE.GetInstanceId();
	}
	return LastIndex;
}

bool FInvContainer::RemoveByInstance(const FGuid& InstanceId, int32 Quantity, int32& OutRemoved)
{
	OutRemoved = 0;
	if (Quantity <= 0)
	{
		return false;
	}
	const int32 Index = FindIndexByInstance(InstanceId);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	FInv_InventoryEntry& E = Entries[Index];
	const int32 ToRemove = FMath::Min(Quantity, E.GetStack());
	E.SetStack(E.GetStack() - ToRemove);
	OutRemoved = ToRemove;
	if (E.GetStack() <= 0)
	{
		Entries.RemoveAt(Index);
		MarkArrayDirty();
	}
	else
	{
		MarkItemDirty(E);
	}
	return true;
}
