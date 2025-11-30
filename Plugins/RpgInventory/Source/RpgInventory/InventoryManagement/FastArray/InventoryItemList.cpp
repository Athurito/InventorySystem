// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemList.h"

#include "RpgInventory/InventoryManagement/Items/InventoryItemDefinition.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryItemFragment.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "GameFramework/Actor.h"


FString FInventoryEntry::GetDebugString() const
{
	return FString();
}

TArray<UInventoryItemInstance*> FInventoryList::GetAllItems() const
{
    TArray<UInventoryItemInstance*> Out;
    Out.Reserve(Entries.Num());
    for (const FInventoryEntry& Entry : Entries)
    {
        if (Entry.Instance)
        {
            Out.Add(Entry.Instance);
        }
    }
    return Out;
}

void FInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
    if (!OwnerComponent) return;
    if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
    {
        for (int32 Idx : RemovedIndices)
        {
            if (Entries.IsValidIndex(Idx))
            {
                Manager->BroadcastSlotChanged(ContainerIndex, Entries[Idx].SlotIndex);
            }
        }
    }
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    if (!OwnerComponent) return;
    if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
    {
        for (int32 Idx : AddedIndices)
        {
            if (Entries.IsValidIndex(Idx))
            {
                Manager->BroadcastSlotChanged(ContainerIndex, Entries[Idx].SlotIndex);
            }
        }
    }
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    if (!OwnerComponent) return;
    if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
    {
        for (int32 Idx : ChangedIndices)
        {
            if (Entries.IsValidIndex(Idx))
            {
                Manager->BroadcastSlotChanged(ContainerIndex, Entries[Idx].SlotIndex);
            }
        }
    }
}

UInventoryItemInstance* FInventoryList::AddEntry(UInventoryItemDefinition* ItemDefinition, int32 SlotIndex, int32 StackCount)
{
    check(ItemDefinition != nullptr);
    check(OwnerComponent);
    
    AActor* OwnerActor = OwnerComponent->GetOwner();
    check(OwnerActor->HasAuthority());
    
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
    if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
    {
        Manager->BroadcastSlotChanged(ContainerIndex, SlotIndex);
    }
    return NewEntry.Instance;
}

void FInventoryList::AddEntry(UInventoryItemInstance* Instance, int32 SlotIndex)
{
    check(OwnerComponent);
    AActor* OwnerActor = OwnerComponent->GetOwner();
    check(OwnerActor->HasAuthority());

    FInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.SlotIndex = SlotIndex;
    NewEntry.Instance = Instance;
    MarkItemDirty(NewEntry);

    if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
    {
        Manager->BroadcastSlotChanged(ContainerIndex, SlotIndex);
    }
}

void FInventoryList::RemoveEntry(UInventoryItemInstance* Instance)
{
    if (!Instance) return;
    int32 RemovedSlot = INDEX_NONE;
    for (int32 i = 0; i < Entries.Num(); ++i)
    {
        if (Entries[i].Instance == Instance)
        {
            RemovedSlot = Entries[i].SlotIndex;
            Entries.RemoveAtSwap(i);
            MarkArrayDirty();
            break;
        }
    }

    if (RemovedSlot != INDEX_NONE)
    {
        if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
        {
            Manager->BroadcastSlotChanged(ContainerIndex, RemovedSlot);
        }
    }
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

void FInventoryList::MoveEntry(int32 SourceSlotIndex, int32 DestSlotIndex)
{
    for (FInventoryEntry& Entry : Entries)
    {
        if (Entry.SlotIndex == SourceSlotIndex)
        {
            const int32 OldSlot = Entry.SlotIndex;
            Entry.SlotIndex = DestSlotIndex;
            MarkItemDirty(Entry);
            BroadcastChangeMessage(Entry, /*OldCount*/ INDEX_NONE, /*NewCount*/ INDEX_NONE);
            return;
        }
    }
}

void FInventoryList::BroadcastChangeMessage(FInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{
}
