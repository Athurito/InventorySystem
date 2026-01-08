// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RpgInventory/InventoryManagement/Container/InventoryContainerDefinition.h"
#include "RpgInventory/InventoryManagement/FastArray/InventoryItemList.h"
#include "InventoryManagerComponent.generated.h"

struct FInvSlotArray;

UENUM(BlueprintType)
enum EInventoryClickAction : uint8
{
	None,
	ContainerSplit,
	ContainerMerge,
	ContainerESort
};

USTRUCT(BlueprintType)
struct FInventoryDragPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<UInventoryManagerComponent> SourceComponent;

	UPROPERTY(BlueprintReadWrite)
	int32 SourceContainerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite)
	int32 SourceSlotIndex = INDEX_NONE;
	
	// UPROPERTY(BlueprintReadWrite)
	// FGuid InstanceId;

	UPROPERTY(BlueprintReadWrite)
	int32 Quantity = 0;
	
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag OperationType = FGameplayTag::EmptyTag;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySlotChanged, int32, ContainerIndex, int32, SlotIndex);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API UInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UInventoryManagerComponent();
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	
	
    void BroadcastSlotChanged(int32 ContainerIndex, int32 SlotIndex) const;

    UInventoryItemInstance* GetItemInstanceInSlot(int32 SlotIndex, int32 ContainerIndex) const;
	
	int32 GetNumContainers() const { return DefaultContainerDefinitions.Num(); }

	int32 GetNumSlots(int32 ContainerIndex) const;

	UPROPERTY(BlueprintAssignable)
	FOnInventorySlotChanged OnInventorySlotChanged;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	int32 TryAddItemDefinition(UInventoryItemDefinition* ItemDefinition, int32 Count);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	UInventoryItemInstance* AddItemDefinition(UInventoryItemDefinition* ItemDefinition, int32 SlotIndex, int32 ContainerIndex, int32 StackCount);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	void AddItemInstance(UInventoryItemInstance* Instance, int32 SlotIndex, int32 ContainerIndex);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	void RemoveItemInstance(UInventoryItemInstance* Instance);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	TArray<UInventoryItemInstance*> GetAllItems(int32 ContainerIndex) const;
	
	UFUNCTION(BlueprintCallable,Category="Inventory")
	void SetInventoryClickAction(EInventoryClickAction Action);
	UFUNCTION(BlueprintCallable,Category="Inventory")
	EInventoryClickAction GetInventoryClickAction() const;
	
	FInventoryList&       GetInventoryList()       { return InventoryList; }
	const FInventoryList& GetInventoryList() const { return InventoryList; }
	
	void QueueSlotRefresh(int32 C, int32 S);
	

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


private:

	friend class UInventoryNetComponent;
	EInventoryClickAction InventoryClickAction = EInventoryClickAction::None;
	
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TArray<TObjectPtr<UInventoryContainerDefinition>> DefaultContainerDefinitions;
	UPROPERTY(Replicated)
	FInventoryList InventoryList;
	
	// UI hooks for client
	UPROPERTY(Transient)
	TSet<int64> PendingSlotRefresh; // key = (Container<<32 | Slot)
	void FlushQueuedSlotRefresh();
	bool bFlushScheduled = false;
	
	
	//Drag drop..
	void HandleDrop_Internal(
		UInventoryManagerComponent* SourceManager,
		int32 SourceContainerIndex,
		int32 SourceSlotIndex,
		int32 TargetContainerIndex,
		int32 TargetSlotIndex,
		int32 DragQuantity,
		FGameplayTag OperationType
	);
	
	void HandleDropSameContainer(
	int32 ContainerIndex,
	int32 SourceSlotIndex,
	int32 DestSlotIndex,
	int32 DragQuantity,
	FGameplayTag OperationType);
	
	void HandleDropDifferentContainer(
	UInventoryManagerComponent* SourceManager,
	int32 SourceContainerIndex,
	int32 SourceSlotIndex,
	int32 TargetContainerIndex,
	int32 TargetSlotIndex,
	int32 DragQuantity,
	FGameplayTag OperationType);
	
	void MoveItemAcrossContainers(
	UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList,
	int32 SourceContainerIndex,
	int32 SourceSlotIndex,
	FInventoryList& DestList,
	int32 DestContainerIndex,
	int32 DestSlotIndex);
	
	void SplitStackAcrossContainers(
	UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList,
	int32 SourceContainerIndex,
	int32 SourceSlotIndex,
	FInventoryList& DestList,
	int32 DestContainerIndex,
	int32 DestSlotIndex,
	int32 SplitQuantity);
	
	void MergeStacksAcrossContainers(
	UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList,
	int32 SourceContainerIndex,
	int32 SourceSlotIndex,
	FInventoryList& DestList,
	int32 DestContainerIndex,
	int32 DestSlotIndex,
	int32 DragQuantity);
	
	void SwapItemsAcrossContainers(
	UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList,
	int32 SourceContainerIndex,
	int32 SourceSlotIndex,
	FInventoryList& DestList,
	int32 DestContainerIndex,
	int32 DestSlotIndex);
	
	
	
	bool CanStack(UInventoryItemInstance* A, UInventoryItemInstance* B) const;
	void MoveItem(FInventoryList& List, int32 ContainerIndex, int32 SourceSlotIndex, int32 TargetSlotIndex);
	void SwapItems(FInventoryList& List, int32 ContainerIndex, int32 SlotA, int32 SlotB);
	void MergeStacks(FInventoryList& List, int32 ContainerIndex, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 DragQuantity);
	void SplitStack(FInventoryList& List, int32 ContainerIndex, int32 SourceSlotIndex, int32 TargetSlotIndex, int32 SplitQuantity);
	
	bool CanPlaceInContainer(UInventoryItemInstance* SourceItem, int32 TargetContainerIndex, int32 TargetSlotIndex) const;
};