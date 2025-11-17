// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemList.h"

#include "RpgInventory/InventoryManagement/Items/InventoryItemDefinition.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryItemFragment.h"


FString FInventoryEntry::GetDebugString() const
{
	return FString();
}

TArray<UInventoryItemInstance*> FInventoryList::GetAllItems() const
{
	return TArray<UInventoryItemInstance*>();
}

void FInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
}

UInventoryItemInstance* FInventoryList::AddEntry(UInventoryItemDefinition* ItemDefinition, int32 SlotIndex, int32 StackCount)
{
	check(ItemDefinition != nullptr)
	check(OwnerComponent)
	
	AActor* OwnerActor = OwnerComponent->GetOwner();
	check(OwnerActor->HasAuthority())
	
	FInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.SlotIndex = SlotIndex;
	AActor* Owner = OwnerComponent->GetOwner();
	NewEntry.Instance = NewObject<UInventoryItemInstance>(Owner);
	NewEntry.Instance->SetItemDef(ItemDefinition);

	for (UInventoryItemFragment* Fragment : ItemDefinition->GetFragments())
	{
		Fragment->OnInstanceCreated(NewEntry.Instance);
		Fragment->OnStackInitialized(NewEntry.Instance, StackCount);
	}
	
	MarkItemDirty(NewEntry);
	return NewEntry.Instance;
}

void FInventoryList::AddEntry(UInventoryItemInstance* Instance, int32 SlotIndex)
{
	unimplemented();
}

void FInventoryList::RemoveEntry(UInventoryItemInstance* Instance)
{
}

UInventoryItemInstance* FInventoryList::GetItemInstanceInSlot(int32 SlotIndex) const
{
	for (const auto& Entry : Entries)
	{
		if (Entry.SlotIndex == SlotIndex) 
		{
			return Entry.Instance;
		}
	}
	return nullptr;
}
