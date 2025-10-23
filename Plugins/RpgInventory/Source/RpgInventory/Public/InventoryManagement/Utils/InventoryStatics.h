// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InventoryStatics.generated.h"

class URpg_ContainerComponent;
class URpg_ItemDefinition;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInventoryStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category="Inventory")
	static URpg_ItemDefinition* GetItemDefinitionById(const FPrimaryAssetId& ItemId);
	static URpg_ContainerComponent* GetContainerComponent(AActor* Owner);
};
