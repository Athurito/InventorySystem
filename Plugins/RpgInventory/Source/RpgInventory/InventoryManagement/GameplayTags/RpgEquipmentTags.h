// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace RpgEquipmentTags
{
	// Slot Tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Equipment_MainHand);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Equipment_OffHand);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Equipment_Head);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Equipment_Chest);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Equipment_Legs);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Equipment_Feet);
	
	// Tool Slots
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Tool_1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Tool_2);

	// Stats / Durability
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Durability);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_MaxDurability);
}
