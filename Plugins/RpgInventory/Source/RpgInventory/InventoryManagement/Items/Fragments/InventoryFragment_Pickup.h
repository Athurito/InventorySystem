// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "InventoryFragment_Pickup.generated.h"

/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInventoryFragment_Pickup : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	FText GetInteractionText() const { return InteractionText; }
	void SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation);
private:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSoftClassPtr<AActor> PickupActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	FText InteractionText;
};
