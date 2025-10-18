// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Rpg_InventoryItem.h"

void URpg_InventoryItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void URpg_InventoryItem::SetItemManifest(const URpg_ItemDefinition& Manifest)
{
}
