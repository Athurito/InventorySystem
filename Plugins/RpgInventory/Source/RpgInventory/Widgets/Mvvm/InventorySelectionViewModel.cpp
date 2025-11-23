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
        UE_MVVM_SET_PROPERTY_VALUE(Rows, 0);
        UE_MVVM_SET_PROPERTY_VALUE(Cols, 0);
        Slots.Reset();
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
        return;
    }

    UE_MVVM_SET_PROPERTY_VALUE(Rows, Manager->GetRows(ContainerIndex));
    UE_MVVM_SET_PROPERTY_VALUE(Cols, Manager->GetCols(ContainerIndex));

    const int32 NumSlots = FMath::Max(0, Rows * Cols);
    Slots.SetNum(NumSlots);
    for (int32 i = 0; i < NumSlots; ++i)
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
            const int32 Qty = Manager->GetQuantityInSlot(i, ContainerIndex);
            Slots[i]->SetFromItemInstance(Instance, Qty);
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
        const int32 Qty = Manager->GetQuantityInSlot(SlotIndex, ContainerIndex);
        VM->SetFromItemInstance(Instance, Qty);
    }
    else
    {
        VM->ClearSlot();
    }

    // Falls UI an das Array als Ganzes bindet
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
}
