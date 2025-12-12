// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace RpgTags
{
	// Container identifiers (cleaner than strings/FNames)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InventoryOperation_SplitOperation);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InventoryOperation_MergeOperation);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InventoryOperation_SortOperation);
}
