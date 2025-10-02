// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Rpg_InventoryComponent.generated.h"

class URpg_ItemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemConsumedSignature, URpg_ItemComponent*, ItemComponent, int32, QuantityUsed);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API URpg_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URpg_InventoryComponent();

	// Client/UI entry point: attempts to consume the given item. Will route to server if needed.
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consume")
	void TryConsumeItem(URpg_ItemComponent* ItemComponent, int32 Quantity /*defaults to 1 in BP*/ = 1);

	// Server authoritative execution
	UFUNCTION(Server, Reliable)
	void ServerConsumeItem(URpg_ItemComponent* ItemComponent, int32 Quantity);

	// Broadcast after successful consumption
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Consume")
	FOnItemConsumedSignature OnItemConsumed;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	bool InternalConsume(URpg_ItemComponent* ItemComponent, int32 Quantity);
};
 