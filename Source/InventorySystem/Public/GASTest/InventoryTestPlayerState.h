// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "InventoryTestPlayerState.generated.h"

class URpg_InventoryComponent;
class UInventoryTestAttributeSet;
class UAbilitySystemComponent;
/**
 * 
 */
UCLASS()
class INVENTORYSYSTEM_API AInventoryTestPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AInventoryTestPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	// Ability System Component used for testing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// Simple AttributeSet for testing inventory-related attributes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UInventoryTestAttributeSet> AttributeSet;
};
