// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryFragment_Durability.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/GameplayTags/RpgEquipmentTags.h"

void UInventoryFragment_Durability::OnInstanceCreated(UInventoryItemInstance* Instance) const
{
	Instance->AddStatTagStack(RpgEquipmentTags::Status_MaxDurability, MaxDurability);
	Instance->AddStatTagStack(RpgEquipmentTags::Status_Durability, MaxDurability);
}
