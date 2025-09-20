#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"

// Native GameplayTags for the RPG Inventory plugin
// Declarations (definitions in Rpg_GameplayTags.cpp)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Weapon_MainHand);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Weapon_OffHand);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Armor_Head);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Armor_Body);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Consumable);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Weapon);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Cooldown_Potion);
