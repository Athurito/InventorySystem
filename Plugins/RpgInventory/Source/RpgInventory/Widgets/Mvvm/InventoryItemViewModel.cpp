// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemViewModel.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Stackable.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Hud.h"

void UInventoryItemViewModel::SetFromItemInstance(UInventoryItemInstance* Instance, UInventoryManagerComponent* InManager, int32 InContainerIndex, int32 InSlotIndex)
{
    ItemInstance = Instance;
    Manager = InManager;
    ContainerIndex = InContainerIndex;
    SlotIndex = InSlotIndex;
    
    UE_MVVM_SET_PROPERTY_VALUE(bIsEmpty, (Instance == nullptr));
    if (bIsEmpty)
    {
        ClearSlot();
        return;
    }
    
    FillFromDefinition();
    FillFromStatTags();
}

void UInventoryItemViewModel::ClearSlot()
{
    UE_MVVM_SET_PROPERTY_VALUE(ItemInstance, nullptr);
    UE_MVVM_SET_PROPERTY_VALUE(CurrentStackCount, 0);
    UE_MVVM_SET_PROPERTY_VALUE(bIsEmpty, true);
}

void UInventoryItemViewModel::FillFromDefinition()
{
    if (ItemInstance == nullptr) return;
    
    if (const UInventoryFragment_Stackable* StackableFragment = ItemInstance->FindFragmentByClass<UInventoryFragment_Stackable>())
    {
        UE_MVVM_SET_PROPERTY_VALUE(MaxStackSize, StackableFragment->GetMaxStackSize());
    }
    
    if (const UInventoryFragment_Hud* HudFragment = ItemInstance->FindFragmentByClass<UInventoryFragment_Hud>())
    {
        UE_MVVM_SET_PROPERTY_VALUE(Icon, HudFragment->GetIconSoft());
    }
}

void UInventoryItemViewModel::FillFromStatTags()
{
    if (ItemInstance == nullptr) return;
    
    UE_MVVM_SET_PROPERTY_VALUE(CurrentStackCount, ItemInstance->GetStatTagStackCount(FragmentTags::StackableFragment));
}
