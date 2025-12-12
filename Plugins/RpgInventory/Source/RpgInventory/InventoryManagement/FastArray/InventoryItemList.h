// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "RpgInventory/InventoryManagement/Container/InventoryContainerDefinition.h"
#include "InventoryItemList.generated.h"

class UInventoryItemDefinition;
struct FInventoryList;
class UInventoryManagerComponent;
class UInventoryItemInstance;
/**
 * 
 */
/** A message when an item is added to the inventory */
USTRUCT(BlueprintType)
struct FLyraInventoryChangeMessage
{
	GENERATED_BODY()

	//@TODO: Tag based names+owning actors for inventories instead of directly exposing the component?
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<UActorComponent> InventoryOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<UInventoryItemInstance> Instance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 NewCount = 0;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 Delta = 0;
};

/** A single entry in an inventory */
USTRUCT(BlueprintType)
struct FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FInventoryEntry()
	{}

	FString GetDebugString() const;

private:
	friend FInventoryList;
	friend UInventoryManagerComponent;

	UPROPERTY()
	TObjectPtr<UInventoryItemInstance> Instance = nullptr;
	
	UPROPERTY(NotReplicated)
	int32 LastObservedCount = INDEX_NONE;
	
	UPROPERTY()
	int32 ContainerIndex = INDEX_NONE;   

	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;
};

/** List of inventory items */
USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FInventoryList() : OwnerComponent(nullptr){}

	FInventoryList(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent){}
	
	
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(Entries, DeltaParms, *this);
	}

	
	TArray<UInventoryItemInstance*> GetAllItemsInContainer(int32 ContainerIndex) const;

	UInventoryItemInstance* AddEntry(UInventoryItemDefinition* ItemDefinition, int32 ContainerIndex, int32 SlotIndex, int32 StackCount);
	void AddEntry(UInventoryItemInstance* Instance, int32 ContainerIndex, int32 SlotIndex);
	void RemoveEntry(UInventoryItemInstance* Instance);

	UInventoryItemInstance* GetItemInstanceInSlot(int32 ContainerIndex, int32 SlotIndex) const;

	void MoveEntry(int32 ContainerIndex, int32 SourceSlotIndex, int32 DestSlotIndex);


private:
    friend UInventoryManagerComponent;
	
    UPROPERTY()
    TArray<FInventoryEntry> Entries;

    UPROPERTY(NotReplicated)
    TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};
