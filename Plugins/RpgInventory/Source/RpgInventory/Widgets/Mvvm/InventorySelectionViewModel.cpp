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
}

void UInventorySelectionViewModel::OnManagerSlotChanged(int32 ChangedContainer, int32 SlotIndex)
{
    if (!Manager.IsValid()) return;
    if (ChangedContainer != ContainerIndex) return;
    if (!Slots.IsValidIndex(SlotIndex)) return;

    UInventoryItemViewModel* VM = Slots[SlotIndex];
    if (!VM) return;

    if (UInventoryItemInstance* Instance = Manager->GetItemInstanceInSlot(SlotIndex, ContainerIndex))
    {
        VM->SetFromItemInstance(Instance, Manager.Get(), ContainerIndex, SlotIndex);
    }
    else
    {
        VM->ClearSlot();
    }

    // Falls UI an das Array als Ganzes bindet
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
}
