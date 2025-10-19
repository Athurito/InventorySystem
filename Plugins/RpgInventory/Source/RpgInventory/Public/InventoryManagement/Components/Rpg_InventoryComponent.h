// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryManagement/FastArray/Rpg_FastArray.h"
#include "Rpg_InventoryComponent.generated.h"

class URpg_ItemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemConsumedSignature, URpg_ItemComponent*, ItemComponent, int32, QuantityUsed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemChange, URpg_InventoryItem*, Item);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API URpg_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URpg_InventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// Client/UI entry point: attempts to consume the given item. Will route to server if needed.
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consume")
	void TryConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity = 1);

	// Server authoritative execution
	UFUNCTION(Server, Reliable)
	void ServerConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity);



	void AddRepSubObject(UObject* SubObject);
	
	// Broadcast after successful consumption
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Consume")
	FOnItemConsumedSignature OnItemConsumed;

	FInventoryItemChange OnItemAdded;
	FInventoryItemChange OnItemRemoved;

protected:
	bool InternalConsume(URpg_ItemComponent* ItemComponent, const int32 Quantity) const; 
private:
	APawn* ResolveInstigator(const URpg_ItemComponent* ItemComponent) const;

	UPROPERTY(Replicated)
	FInv_InventoryFastArray InventoryList;
};
 