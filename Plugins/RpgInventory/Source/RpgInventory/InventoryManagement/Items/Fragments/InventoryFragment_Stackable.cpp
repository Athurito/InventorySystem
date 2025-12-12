// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryFragment_Stackable.h"

#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"


void UInventoryFragment_Stackable::OnStackInitialized(UInventoryItemInstance* Instance, int32 StackCount) const
{
	Instance->AddStatTagStack(FragmentTags::StackableFragment, StackCount);
}
