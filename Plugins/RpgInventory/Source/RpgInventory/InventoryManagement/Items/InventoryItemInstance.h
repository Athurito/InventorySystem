// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagStack.h"
#include "UObject/Object.h"
#include "InventoryItemInstance.generated.h"

class UInventoryItemFragment;
class UInventoryItemDefinition;
struct FGameplayTag;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInventoryItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UInventoryItemInstance();

	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	//~End of UObject interface

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category= Inventory)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Inventory)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Inventory)
	bool HasStatTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UInventoryItemFragment* FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const;

	
	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return Cast<ResultClass>(FindFragmentByClass(ResultClass::StaticClass()));
	}

	UInventoryItemDefinition* GetItemDefinition() const
	{
		return ItemDef;
	}

private:
	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	// The item definition
	UPROPERTY(Replicated)
	TObjectPtr<UInventoryItemDefinition> ItemDef;

	void SetItemDef(UInventoryItemDefinition* InDef);
	friend struct FInventoryList;
};