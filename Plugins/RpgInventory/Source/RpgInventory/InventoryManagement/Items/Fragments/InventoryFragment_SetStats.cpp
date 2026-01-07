// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryFragment_SetStats.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"

void UInventoryFragment_SetStats::OnInstanceCreated(UInventoryItemInstance* Instance) const
{
	for (const auto& KVP : InitialStatTags)
	{
		Instance->AddStatTagStack(KVP.Key, KVP.Value);
	}
}

int32 UInventoryFragment_SetStats::GetStatTagStackCount(FGameplayTag Tag) const
{
	if (const int32* Ptr = InitialStatTags.Find(Tag))
	{
		return *Ptr;
	}
	return 0;
}
