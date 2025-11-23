// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemViewModel.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"

void UInventoryItemViewModel::SetFromItemInstance(UInventoryItemInstance* Instance, int32 InQuantity)
{
    UE_MVVM_SET_PROPERTY_VALUE(ItemInstance, Instance);
    UE_MVVM_SET_PROPERTY_VALUE(Quantity, FMath::Max(0, InQuantity));
    const bool bNowEmpty = (Instance == nullptr);
    UE_MVVM_SET_PROPERTY_VALUE(bIsEmpty, bNowEmpty);
}

void UInventoryItemViewModel::ClearSlot()
{
    UE_MVVM_SET_PROPERTY_VALUE(ItemInstance, nullptr);
    UE_MVVM_SET_PROPERTY_VALUE(Quantity, 0);
    UE_MVVM_SET_PROPERTY_VALUE(bIsEmpty, true);
}
