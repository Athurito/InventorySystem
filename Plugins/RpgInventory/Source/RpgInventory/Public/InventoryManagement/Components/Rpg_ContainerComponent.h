// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryManagement/FastArray/Rpg_FastArray.h"
#include "UObject/PrimaryAssetId.h"
#include "Rpg_ContainerComponent.generated.h"


USTRUCT(BlueprintType)
struct FInvContainerEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EInventorySlotType Type = EInventorySlotType::Generic;
	UPROPERTY(BlueprintReadOnly) int32 Index = INDEX_NONE; // Index im Containers-Array
};

class URpg_ItemComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemConsumedSignature, URpg_ItemComponent*, ItemComponent, int32, QuantityUsed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemChange, FInv_InventoryEntry, Item);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API URpg_ContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URpg_ContainerComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// Initial Container-Defs (im Editor/Blueprint setzen)
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TArray<TSoftObjectPtr<UInventoryContainerDefinition>> InitialContainerDefs;

	// Runtime Container (Meta + FastArray)
	UPROPERTY(Replicated) TArray<FInvContainer> Containers;

	/** Add / Remove / Transfer **/
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool AddItemToContainer(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool AddItemToContainerById(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool RemoveItemFromContainer(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool TransferItem(URpg_ContainerComponent* TargetComponent, int32 SourceContainerIndex, int32 TargetContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutMoved);

	UFUNCTION(Server, Reliable)
	void ServerAddItemToContainer(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity);
	UFUNCTION(Server, Reliable)
	void ServerAddItemToContainerById(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity);
	UFUNCTION(Server, Reliable)
	void ServerRemoveItemFromContainer(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity);
	UFUNCTION(Server, Reliable)
	void ServerTransferItem(URpg_ContainerComponent* TargetComponent, int32 SourceContainerIndex, int32 TargetContainerIndex, const FGuid& InstanceId, int32 Quantity);

	/** Consumption **/
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consume")
	void TryConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity = 1);

	// Server authoritative execution
	UFUNCTION(Server, Reliable)
	void ServerConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity);
	
	
	// Broadcast after successful consumption
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Consume")
	FOnItemConsumedSignature OnItemConsumed;
	
	FInventoryItemChange OnItemAdded;
	FInventoryItemChange OnItemRemoved;
	
	void AddRepSubObject(UObject* SubObject);
protected:
	bool InternalConsume(URpg_ItemComponent* ItemComponent, const int32 Quantity) const;

	virtual void BeginPlay() override;
private:
	APawn* ResolveInstigator(const URpg_ItemComponent* ItemComponent) const;
	bool InternalAddItem(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId);
	bool InternalAddItemById(int32 ContainerIndex, const FPrimaryAssetId& ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId);
	bool InternalRemoveItem(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved);
	bool InternalTransferItem(URpg_ContainerComponent* TargetComponent, int32 SourceContainerIndex, int32 TargetContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutMoved);

	UPROPERTY(Replicated)
	FInvContainer InventoryList;
};
 