// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InventoryStatics.generated.h"

class UAbilitySystemComponent;
class UInventoryManagerComponent;
class UInventoryItemDefinition;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInventoryStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category="Inventory")
	static UInventoryItemDefinition* GetItemDefinitionById(const FPrimaryAssetId& ItemId);
	static UInventoryManagerComponent* GetContainerComponent(AActor* Owner);
	static UInventoryManagerComponent* ResolveInventoryFromInstigator(APawn* Instigator);
	static UAbilitySystemComponent* ResolveASCFromPawn(APawn* InstigatorPawn);
};
