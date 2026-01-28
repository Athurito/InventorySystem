// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySelectionViewModel.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/Widgets/Mvvm/InventoryItemViewModel.h"

void UInventorySelectionViewModel::SetSelectedItem(UInventoryItemViewModel* Item)
{
    if (!Item || SelectedItem == Item) return;
    UE_MVVM_SET_PROPERTY_VALUE(SelectedItem, Item);
}

void UInventorySelectionViewModel::InitializeFromManager(UInventoryManagerComponent* InManager, int32 InContainerIndex)
{
    // Verhindere unnötiges Re-Initialize, wenn Manager und Index gleich sind
    if (Manager == InManager && ContainerIndex == InContainerIndex)
    {
        return;
    }

    if (Manager.IsValid())
    {   
        Manager->OnInventorySlotChanged.RemoveDynamic(this, &UInventorySelectionViewModel::OnManagerSlotChanged);
    }
    
    Manager = InManager;
    ContainerIndex = InContainerIndex;

    if (!Manager.IsValid())
    {
        UE_MVVM_SET_PROPERTY_VALUE(TotalSlots, 0);
        Slots.Reset();
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
        return;
    }

    UE_MVVM_SET_PROPERTY_VALUE(TotalSlots, Manager->GetNumSlots(ContainerIndex));
    
    Slots.SetNum(TotalSlots);
    for (int32 i = 0; i < TotalSlots; ++i)
    {
        if (!Slots[i])
        {
            UInventoryItemViewModel* VM = NewObject<UInventoryItemViewModel>(this);
            // Kontext optional setzen (private Freunde erlaubt)
            VM->Manager = Manager.Get();
            VM->ContainerIndex = ContainerIndex;
            VM->SlotIndex = i;
            Slots[i] = VM;
        }

        if (UInventoryItemInstance* Instance = Manager->GetItemInstanceInSlot(i, ContainerIndex))
        {
            Slots[i]->SetFromItemInstance(Instance, Manager.Get(), ContainerIndex, i);
        }
        else
        {
            Slots[i]->ClearSlot();
        }
    }
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);

    // Abonniere Manager-Events
    if (Manager.IsValid())
    {   
        Manager->OnInventorySlotChanged.AddUniqueDynamic(this, &UInventorySelectionViewModel::OnManagerSlotChanged);
    }
    else
    {
        // Wenn kein Manager da ist, stellen wir sicher, dass wir leer sind (Cleanup)
        Slots.Empty();
        UE_MVVM_SET_PROPERTY_VALUE(TotalSlots, 0);
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
    }
}

void UInventorySelectionViewModel::OnManagerSlotChanged(int32 ChangedContainer, int32 SlotIndex)
{
    if (!Manager.IsValid()) return;
    if (ChangedContainer != ContainerIndex) return;

    const int32 NumSlotsNow = Manager->GetNumSlots(ContainerIndex);
    if (Slots.Num() != NumSlotsNow)
    {
        InitializeFromManager(Manager.Get(), ContainerIndex);
        return;
    }

    // *** Hard-Resync aller Slots – simpel und robust ***
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (!Slots[i]) continue;

        if (UInventoryItemInstance* Instance = Manager->GetItemInstanceInSlot(i, ContainerIndex))
        {
            Slots[i]->SetFromItemInstance(Instance, Manager.Get(), ContainerIndex, i);
        }
        else
        {
            Slots[i]->ClearSlot();
        }
    }

    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
}
