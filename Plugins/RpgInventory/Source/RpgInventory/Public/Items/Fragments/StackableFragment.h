// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Fragments/ItemFragment.h"
#include "Items/Fragments/Rpg_FragmentTags.h"
#include "StackableFragment.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FStackableFragment : public FItemFragment
{
	GENERATED_BODY()

	FStackableFragment()
	{
		SetFragmentTag(FragmentTags::StackableFragment);
	}
	int32 GetMaxStackSize() const { return MaxStackSize; }
	int32 GetStackCount() const { return StackCount; }
	void SetStackCount(const int32 Count) { StackCount = Count; }

private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 MaxStackSize{1};
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 StackCount{1};
};
