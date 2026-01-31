// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace RpgTags
{
	// Generic "use item" (fallback)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InventoryEvent_UseItem);

	// Fired when a non-consumable equippable is used (e.g. from hotbar)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InventoryEvent_UseEquippable);
}
