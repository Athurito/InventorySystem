// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "InventoryItemDefinition.generated.h"

class UInventoryItemFragment;
/**
 * 
 */
UCLASS(Const, Abstract)
class RPGINVENTORY_API UInventoryItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Item"), GetFName());
	}
	
	const FGameplayTag& GetItemType() const { return ItemType; }
	
	UInventoryItemFragment* FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const;
	
private:

	UPROPERTY(EditDefaultsOnly, Category="Inventory", Instanced)
	TArray<TObjectPtr<UInventoryItemFragment>> Fragments;

	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (Categories="GameItems"))
	FGameplayTag ItemType;
};
