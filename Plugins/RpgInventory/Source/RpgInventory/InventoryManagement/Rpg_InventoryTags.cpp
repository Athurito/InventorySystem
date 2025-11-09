// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Rpg_InventoryTags.h"


namespace RpgTags
{
	UE_DEFINE_GAMEPLAY_TAG(Container_Inventory, TEXT("Inventory.Main"));
	UE_DEFINE_GAMEPLAY_TAG(Container_Hotbar, TEXT("Inventory.Hotbar"));
	UE_DEFINE_GAMEPLAY_TAG(Container_Bag, TEXT("Inventory.Bag"));
	UE_DEFINE_GAMEPLAY_TAG(Container_Equipment, TEXT("Inventory.Equipment"));


	UE_DEFINE_GAMEPLAY_TAG(Item_Consumable_HealthPotion, TEXT("Item.Consumable.HealthPotion"));
	UE_DEFINE_GAMEPLAY_TAG(Item_Resource_Wood, TEXT("Item.Resource.Wood"));
}
