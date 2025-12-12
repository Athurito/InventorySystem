// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryNetComponent.generated.h"


struct FGameplayTag;
class UInventoryManagerComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API UInventoryNetComponent : public UActorComponent
{
	GENERATED_BODY()

	
public:
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void Server_HandleDrop(
		UInventoryManagerComponent* SourceManager,
		int32 SourceContainerIndex,
		int32 SourceSlotIndex,
		UInventoryManagerComponent* TargetManager,
		int32 TargetContainerIndex,
		int32 TargetSlotIndex,
		int32 DragQuantity,
		FGameplayTag OperationType);
};
