// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Rpg_FragmentTags.h"
#include "UObject/Object.h"
#include "ItemFragment.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FItemFragment
{
	GENERATED_BODY()

	FItemFragment() {  }
	FItemFragment(const FItemFragment&) = default;
	FItemFragment& operator=(const FItemFragment&) = default;
	FItemFragment(FItemFragment&&) = default;
	FItemFragment& operator=(FItemFragment&&) = default;
	virtual ~FItemFragment() {}


	FGameplayTag GetFragmentTag() const { return FragmentTag; }
	void SetFragmentTag(const FGameplayTag Tag) { FragmentTag = Tag; }
	virtual void Manifest() {}
	
private:
	
	FGameplayTag FragmentTag = FGameplayTag::EmptyTag;
};

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