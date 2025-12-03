// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RpgInventory/InventoryManagement/Container/InventoryContainerDefinition.h"
#include "RpgInventory/InventoryManagement/FastArray/InventoryItemList.h"
#include "InventoryManagerComponent.generated.h"

struct FInvSlotArray;

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
	bool bIsShiftDown = false;
};

USTRUCT()
struct FInventoryContainerInstance
{
	GENERATED_BODY()
	
	UPROPERTY()
	TObjectPtr<const UInventoryContainerDefinition> Definition = nullptr;
	
	UPROPERTY()
	FInventoryList InventoryList;
	
	int32 GetNumSlots() const
	{
		return Definition ? Definition->TotalSlots : 0;
	}

	bool IsValidSlot(int32 SlotIndex) const
	{
		return SlotIndex >= 0 && SlotIndex < GetNumSlots();
	}
};

USTRUCT(BlueprintType)
struct FInvContainerEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EInventorySlotType Type = EInventorySlotType::Generic;
	UPROPERTY(BlueprintReadOnly) int32 Index = INDEX_NONE; // Index im Containers-Array
};

// Informiert UI/ViewModels darüber, dass sich ein Slot in einem bestimmten Container geändert hat
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySlotChanged, int32, ContainerIndex, int32, SlotIndex);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API UInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UInventoryManagerComponent();
    
    void InitializeContainers();
    void AddRepSubObject(UObject* SubObject);
	
    void BroadcastSlotChanged(int32 ContainerIndex, int32 SlotIndex) const;

    UInventoryItemInstance* GetItemInstanceInSlot(int32 SlotIndex, int32 ContainerIndex) const;
	
    int32 GetNumContainers() const { return Containers.Num(); }
    int32 GetNumSlots(int32 ContainerIndex) const
    {
        return (Containers.IsValidIndex(ContainerIndex) && Containers[ContainerIndex].Definition)
            ? Containers[ContainerIndex].GetNumSlots()
            : 0;
    }
    // Aktuelle Menge im Slot (falls keine Stack‑Info vorhanden, mindestens 1 bei belegtem Slot)
    int32 GetQuantityInSlot(int32 SlotIndex, int32 ContainerIndex) const;

	UPROPERTY(BlueprintAssignable)
	FOnInventorySlotChanged OnInventorySlotChanged;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	UInventoryItemInstance* AddItemDefinition(UInventoryItemDefinition* ItemDefinition, int32 SlotIndex, int32 ContainerIndex, int32 StackCount);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	void AddItemInstance(UInventoryItemInstance* Instance, int32 SlotIndex, int32 ContainerIndex);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	void RemoveItemInstance(UInventoryItemInstance* Instance, int32 ContainerIndex);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	TArray<UInventoryItemInstance*> GetAllItems(int32 ContainerIndex) const;
	
	
	//Drag drop..
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|DragDrop")
	void HandleDrop(
		UInventoryManagerComponent* SourceManager,
		int32 SourceContainerIndex,
		int32 SourceSlotIndex,
		int32 TargetContainerIndex,
		int32 TargetSlotIndex,
		int32 DragQuantity,
		bool bSplitStackIfPossible
	);
	
	

	

protected:
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TArray<TObjectPtr<UInventoryContainerDefinition>> DefaultContainerDefinitions;

	UPROPERTY(Replicated)
	TArray<FInventoryContainerInstance> Containers;
	
	void HandleDropSameContainer(
	int32 ContainerIndex,
	int32 SourceSlotIndex,
	int32 DestSlotIndex,
	int32 DragQuantity,
	bool bSplitStackIfPossible);
	
	void HandleDropDifferentContainer(
	UInventoryManagerComponent* SourceManager,
	int32 SourceContainerIndex,
	int32 SourceSlotIndex,
	int32 DestContainerIndex,
	int32 DestSlotIndex,
	int32 DragQuantity,
	bool bSplitStackIfPossible);
	
	
	
	bool CanStack(UInventoryItemInstance* A, UInventoryItemInstance* B) const;
	void MoveItem(FInventoryList& List, int32 SourceSlotIndex, int32 TargetSlotIndex);
	void SwapItems(FInventoryList& List, int32 SlotA, int32 SlotB);
	void MergeStacks(FInventoryList& List, int32 SourceSlotIndex, int32 TargetSlotIndex,  int32 DragQuantity);
	void SplitStack(FInventoryList& List,int32 SourceSlotIndex,int32 TargetSlotIndex,int32 SplitQuantity);
	
	const FInventoryList& GetInventoryList(int32 ContainerIndex) const;
	FInventoryList& GetInventoryList(int32 ContainerIndex);
	
};