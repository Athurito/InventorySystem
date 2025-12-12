// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "InventoryTestAttributeSet.generated.h"

/**
 * Minimal AttributeSet for GAS testing with inventory systems.
 * Demonstrates use of UE5.6 ATTRIBUTE_ACCESSORS_BASIC macro.
 */
UCLASS()
class INVENTORYSYSTEM_API UInventoryTestAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UInventoryTestAttributeSet() = default;

	// UAttributeSet interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Example attributes that could be useful for inventory testing
	// Total carry capacity a player can hold
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CarryCapacity, Category = "Attributes|Inventory")
	FGameplayAttributeData CarryCapacity;
	ATTRIBUTE_ACCESSORS_BASIC(UInventoryTestAttributeSet, CarryCapacity);

	// Current accumulated carried weight
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentWeight, Category = "Attributes|Inventory")
	FGameplayAttributeData CurrentWeight;
	ATTRIBUTE_ACCESSORS_BASIC(UInventoryTestAttributeSet, CurrentWeight);

protected:
	UFUNCTION()
	void OnRep_CarryCapacity(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CurrentWeight(const FGameplayAttributeData& OldValue);
};
