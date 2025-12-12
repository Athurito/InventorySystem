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

void FInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
    if (!OwnerComponent) return;

    if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
    {
        AActor* OwnerActor = Manager->GetOwner();
        if (OwnerActor && OwnerActor->HasAuthority())
        {
            // Server: macht das Remove-Event schon in RemoveEntry
            return;
        }

        for (int32 Idx : RemovedIndices)
        {
            if (Entries.IsValidIndex(Idx))
            {
                FInventoryEntry& E = Entries[Idx];

                const int32 CIdx = E.ContainerIndex;
                const int32 SIdx = E.SlotIndex;

                // *** WICHTIG: Instanz auf dem Client vor dem Broadcast nullen ***
                E.Instance = nullptr;

                Manager->BroadcastSlotChanged(CIdx, SIdx);
            }
        }
    }
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    if (!OwnerComponent) return;

    if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
    {
        AActor* OwnerActor = Manager->GetOwner();
        if (OwnerActor && OwnerActor->HasAuthority())
        {
            // Server hat schon in Add/Remove/Move gebroadcastet → hier nix tun
            return;
        }
        for (int32 Idx : AddedIndices)
        {
            if (Entries.IsValidIndex(Idx))
            {
                const FInventoryEntry& E = Entries[Idx];
                UE_LOG(LogTemp, Warning, TEXT("Client PostAdd: C=%d S=%d Instance=%s"),
                    E.ContainerIndex,
                    E.SlotIndex,
                    E.Instance ? *E.Instance->GetName() : TEXT("NULL"));
                const int32 CIdx = Entries[Idx].ContainerIndex;
                const int32 SIdx = Entries[Idx].SlotIndex;
                
                Manager->BroadcastSlotChanged(CIdx, SIdx);
            }
        }
    }
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    if (!OwnerComponent) return;

    if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
    {
        AActor* OwnerActor = Manager->GetOwner();
        if (OwnerActor && OwnerActor->HasAuthority())
        {
            // Server hat schon in Add/Remove/Move gebroadcastet → hier nix tun
            return;
        }
        for (int32 Idx : ChangedIndices)
        {
            if (Entries.IsValidIndex(Idx))
            {
                const int32 CIdx = Entries[Idx].ContainerIndex;
                const int32 SIdx = Entries[Idx].SlotIndex;
                Manager->BroadcastSlotChanged(CIdx, SIdx);
            }
        }
    }
}

TArray<UInventoryItemInstance*> FInventoryList::GetAllItemsInContainer(int32 ContainerIndex) const
{
    TArray<UInventoryItemInstance*> Out;
    for (const FInventoryEntry& Entry : Entries)
    {
        if (Entry.ContainerIndex == ContainerIndex && Entry.Instance)
        {
            Out.Add(Entry.Instance);
        }
    }
    return Out;
}

UInventoryItemInstance* FInventoryList::AddEntry(UInventoryItemDefinition* ItemDefinition, int32 ContainerIndex, int32 SlotIndex, int32 StackCount)
{
    check(ItemDefinition != nullptr);
    check(OwnerComponent);

    AActor* OwnerActor = OwnerComponent->GetOwner();
    check(OwnerActor && OwnerActor->HasAuthority());

    FInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.ContainerIndex = ContainerIndex;
    NewEntry.SlotIndex      = SlotIndex;

    NewEntry.Instance = NewObject<UInventoryItemInstance>(OwnerActor);
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

void FInventoryList::AddEntry(UInventoryItemInstance* Instance, int32 ContainerIndex, int32 SlotIndex)
{
    check(OwnerComponent);
    AActor* OwnerActor = OwnerComponent->GetOwner();
    check(OwnerActor && OwnerActor->HasAuthority());

    FInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
    NewEntry.ContainerIndex = ContainerIndex;
    NewEntry.SlotIndex      = SlotIndex;
    NewEntry.Instance       = Instance;

    MarkItemDirty(NewEntry);

    if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
    {
        Manager->BroadcastSlotChanged(ContainerIndex, SlotIndex);
    }
}

void FInventoryList::RemoveEntry(UInventoryItemInstance* Instance)
{
    if (!Instance) return;

    int32 RemovedContainer = INDEX_NONE;
    int32 RemovedSlot      = INDEX_NONE;

    for (int32 i = 0; i < Entries.Num(); ++i)
    {
        if (Entries[i].Instance == Instance)
        {
            RemovedContainer = Entries[i].ContainerIndex;
            RemovedSlot      = Entries[i].SlotIndex;

            Entries.RemoveAtSwap(i);
            MarkArrayDirty();
            break;
        }
    }

    if (RemovedSlot != INDEX_NONE)
    {
        if (UInventoryManagerComponent* Manager = Cast<UInventoryManagerComponent>(OwnerComponent))
        {
            Manager->BroadcastSlotChanged(RemovedContainer, RemovedSlot);
        }
    }
}

UInventoryItemInstance* FInventoryList::GetItemInstanceInSlot(int32 ContainerIndex, int32 SlotIndex) const
{
    for (const FInventoryEntry& Entry : Entries)
    {
        if (Entry.ContainerIndex == ContainerIndex && Entry.SlotIndex == SlotIndex)
        {
            return Entry.Instance;
        }
    }
    return nullptr;
}

void FInventoryList::MoveEntry(int32 ContainerIndex, int32 SourceSlotIndex, int32 DestSlotIndex)
{
    if (SourceSlotIndex == DestSlotIndex)
    {
        return;
    }

    // Instanz im Source-Slot holen
    UInventoryItemInstance* Instance = GetItemInstanceInSlot(ContainerIndex, SourceSlotIndex);
    if (!Instance)
    {
        return;
    }

    // 1) Eintrag entfernen → löst Remove-Events aus (Server + Client)
    RemoveEntry(Instance);

    // 2) Gleiche Instanz im Ziel-Slot wieder einfügen → löst Add-Events aus
    AddEntry(Instance, ContainerIndex, DestSlotIndex);
}
