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
    
    FillFromContainerDefinition();

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
    UE_MVVM_SET_PROPERTY_VALUE(DisplayName, FText::GetEmpty());
    UE_MVVM_SET_PROPERTY_VALUE(Description, FText::GetEmpty());
    UE_MVVM_SET_PROPERTY_VALUE(Icon, TSoftObjectPtr<UTexture2D>()); // leerer SoftPtr
    UE_MVVM_SET_PROPERTY_VALUE(DurabilityCurrent, 0);
    UE_MVVM_SET_PROPERTY_VALUE(DurabilityMax, 0);
    UE_MVVM_SET_PROPERTY_VALUE(CurrentStackCount, 0);
    UE_MVVM_SET_PROPERTY_VALUE(MaxStackSize, 0);
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
        UE_MVVM_SET_PROPERTY_VALUE(DisplayName, HudFragment->DisplayName);
        UE_MVVM_SET_PROPERTY_VALUE(Description, HudFragment->Description);
    }
}

void UInventoryItemViewModel::FillFromStatTags()
{
    if (ItemInstance == nullptr) return;
    
    UE_MVVM_SET_PROPERTY_VALUE(CurrentStackCount, ItemInstance->GetStatTagStackCount(FragmentTags::StackableFragment));
}

void UInventoryItemViewModel::FillFromContainerDefinition()
{
    if (!Manager.IsValid()) return;
    UInventoryContainerDefinition* Def = Manager->GetContainerDefinition(ContainerIndex);
    if (!Def) return;

    if (Def->SlotDefinitions.IsValidIndex(SlotIndex))
    {
        UE_MVVM_SET_PROPERTY_VALUE(BackgroundIcon, Def->SlotDefinitions[SlotIndex].BackgroundIcon);
    }
    else
    {
        UE_MVVM_SET_PROPERTY_VALUE(BackgroundIcon, TSoftObjectPtr<UTexture2D>());
    }
}
