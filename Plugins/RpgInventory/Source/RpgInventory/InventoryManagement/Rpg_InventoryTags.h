// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace RpgTags
{
	// Container identifiers (cleaner than strings/FNames)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Container_Inventory);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Container_Hotbar);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Container_Bag);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Container_Equipment);


	// Item identifiers (examples; extend for your game)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Consumable_HealthPotion);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Resource_Wood);
}
