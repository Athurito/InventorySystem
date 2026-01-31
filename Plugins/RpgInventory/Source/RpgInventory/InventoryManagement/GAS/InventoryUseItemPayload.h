// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Consumable.h"

#include "InventoryUseItemPayload.generated.h"

class UInventoryManagerComponent;
class UInventoryItemInstance;

UCLASS(BlueprintType)
class RPGINVENTORY_API UInventoryUseItemPayload : public UObject
{
	GENERATED_BODY()

public:
	/** Manager that owns the containers/slots (typically lives on PlayerState). */
	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	TWeakObjectPtr<UInventoryManagerComponent> InventoryManager;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UInventoryItemInstance> ItemInstance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	int32 ContainerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	EUseContext UseContext = EUseContext::Inventory;
};
