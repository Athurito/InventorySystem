#include "Rpg_GameplayTags.h"
#include "NativeGameplayTags.h"
#include "GameplayTagsManager.h"

// Define native GameplayTags. These names become the canonical tag paths in the project.
// Ensure your DefaultGameplayTags.ini (or project settings) allows dynamic creation or these exist in the tag list.

UE_DEFINE_GAMEPLAY_TAG(TAG_Slot, TEXT("Slot"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_Weapon_MainHand, TEXT("Slot.Weapon.MainHand"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_Weapon_OffHand, TEXT("Slot.Weapon.OffHand"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_Armor_Head, TEXT("Slot.Armor.Head"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_Armor_Body, TEXT("Slot.Armor.Body"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Consumable, TEXT("Item.Consumable"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Weapon, TEXT("Item.Weapon"));

UE_DEFINE_GAMEPLAY_TAG(TAG_State_Cooldown_Potion, TEXT("State.Cooldown.Potion"));
