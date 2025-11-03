// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "InventoryManagement/FastArray/Rpg_FastArray.h"
#include "UObject/PrimaryAssetId.h"
#include "Rpg_ContainerComponent.generated.h"

struct FInvSlotArray;

USTRUCT(BlueprintType)
struct FInventoryDragPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<class URpg_ContainerComponent> SourceComponent;

	UPROPERTY(BlueprintReadWrite)
	int32 SourceContainerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite)
	int32 SourceSlotIndex = INDEX_NONE;
	
	UPROPERTY(BlueprintReadWrite)
	FGuid InstanceId;

	UPROPERTY(BlueprintReadWrite)
	int32 Quantity = 0; // 0 or less means all
};

USTRUCT()
struct FContainerSlotMap
{
	GENERATED_BODY()
	UPROPERTY() TArray<FGuid> SlotToInstance; // Größe = Rows*Cols, FGuid() == leer
};

// Forward declarations to avoid heavy includes
enum class EInventorySlotType : uint8;
enum class EUseContext : uint8;
class UInventoryContainerDefinition;
class UAbilitySystemComponent;
class APawn;
struct FConsumableFragment;
class URpg_ItemComponent;
class URpg_ItemDefinition;

USTRUCT(BlueprintType)
struct FInvContainerEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EInventorySlotType Type = EInventorySlotType::Generic;
	UPROPERTY(BlueprintReadOnly) int32 Index = INDEX_NONE; // Index im Containers-Array
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemConsumedSignature, URpg_ItemComponent*, ItemComponent, int32, QuantityUsed);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSlotChanged, int32, ContainerIndex, int32, SlotIndex, FGuid, InstanceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEntryChanged, FGuid, InstanceId, const FInv_InventoryEntry&, Entry);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API URpg_ContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URpg_ContainerComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;






	// Präzises UI-Signal pro Slot
	UPROPERTY(BlueprintAssignable, Category="Inventory|Container")
	FOnSlotChanged OnSlotChanged;

	// Präzises UI-Signal pro Slot
	UPROPERTY(BlueprintAssignable, Category="Inventory|Container")
	FOnEntryChanged OnEntryChanged;

	// Replizierte Slot-Zustände (parallel zu Containers)
	UPROPERTY(ReplicatedUsing=OnRep_Slots)
	TArray<FInvSlotArray> Slots;

	// UI-Broadcast-Hook (ruft OnSlotChanged mit ContainerIndex zurück)
	void BroadcastSlotChangedForOwner(const FInvSlotArray* OwnerArray, int32 SlotIndex, const FGuid& InstanceId);


	

	// Initial Container-Defs (im Editor/Blueprint setzen)
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TArray<TSoftObjectPtr<UInventoryContainerDefinition>> InitialContainerDefs;

	int32 GetContainerCount() const;
	
	// Runtime Container (Meta + FastArray)
	UPROPERTY(ReplicatedUsing=OnRep_Containers) TArray<FInvContainer> Containers;

	/** Query helpers for Blueprints (Drag&Drop/UI) **/
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	bool GetEntryAtIndex(int32 ContainerIndex, int32 EntryIndex, FInv_InventoryEntry& OutEntry) const;
	UFUNCTION(BlueprintPure, Category = "Inventory|Query")
	bool FindIndexByInstance(int32 ContainerIndex, const FGuid& InstanceId, int32& OutEntryIndex) const;

	/** Add / Remove / Transfer **/
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool AddItemToContainer(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool AddItemToContainerById(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool RemoveItemFromContainer(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool TransferItem(const FInventoryDragPayload& Payload,
	URpg_ContainerComponent* TargetComponent,
	int32 TargetContainerIndex,
	int32 TargetSlotIndex,
	int32& OutMoved);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool TransferFromPayloadTo(const FInventoryDragPayload& Payload,
	URpg_ContainerComponent* TargetComponent,
	int32 TargetContainerIndex,
	int32 TargetSlotIndex,
	int32& OutMoved);
	// Auto-deposit only items that already exist in target container (hotkey support)
	UFUNCTION(BlueprintCallable, Category = "Inventory|Container")
	bool AutoDepositMatchingTo(URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex, int32& OutTotalMoved);

	

	UFUNCTION(Server, Reliable)
	void ServerAddItemToContainer(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity);
	UFUNCTION(Server, Reliable)
	void ServerAddItemToContainerById(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity);
	UFUNCTION(Server, Reliable)
	void ServerRemoveItemFromContainer(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity);
	UFUNCTION(Server, Reliable)
	void ServerTransferItem(const FInventoryDragPayload& Payload,
	URpg_ContainerComponent* TargetComponent,
	int32 TargetContainerIndex,
	int32 TargetSlotIndex);
	UFUNCTION(Server, Reliable)
	void ServerAutoDepositMatchingTo(URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex);


	/** Swap support for Drag&Drop **/
	UFUNCTION(BlueprintCallable, Category = "Inventory|Transfer")
	bool SwapSlots(URpg_ContainerComponent* OtherComponent,
		int32 ThisContainerIndex, int32 ThisSlotIndex,
		int32 OtherContainerIndex, int32 OtherSlotIndex);
	
	

	/** Consumption **/
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consume")
	void TryConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity = 1);

	// New unified Use API
	UFUNCTION(BlueprintCallable, Category = "Inventory|Use")
	void TryUseItemByInstance(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity = 1);
	UFUNCTION(BlueprintCallable, Category = "Inventory|Use")
	void TryUseWorldItem(URpg_ItemComponent* ItemComponent, int32 Quantity = 1);

	// Server authoritative execution
	UFUNCTION(Server, Reliable)
	void ServerConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity);
	UFUNCTION(Server, Reliable)
	void ServerUseItemByInstance(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity);
	UFUNCTION(Server, Reliable)
	void ServerUseWorldItem(URpg_ItemComponent* ItemComponent, int32 Quantity);


	UInventoryContainerDefinition* GetContainerDefinition(const int32 Index) const;
	
	// Broadcast after successful consumption
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Consume")
	FOnItemConsumedSignature OnItemConsumed;
	
	// Helper:
	const FInv_InventoryEntry* GetEntryBySlot(int32 ContainerIdx, int32 SlotIdx) const;
	FInv_InventoryEntry* GetEntryBySlotMutable(int32 ContainerIdx, int32 SlotIdx);
	
	void AddRepSubObject(UObject* SubObject);
protected:
	bool InternalConsume(URpg_ItemComponent* ItemComponent, const int32 Quantity) const;
	bool InternalUseItem_Inventory(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity);
	bool InternalUseItem_World(URpg_ItemComponent* ItemComponent, int32 Quantity);

	UFUNCTION()
	void OnRep_Slots();
	void InitSlotsArrayFromContainers(); // initiales Setup/Resize & Owner setzen

	// Slot Helpers
	FGuid GetSlotInstance(int32 ContainerIdx, int32 SlotIdx) const;
	void SetSlotInstance(int32 ContainerIdx, int32 SlotIdx, const FGuid& InstanceId);
	void ClearSlot(int32 ContainerIdx, int32 SlotIdx);

	virtual void BeginPlay() override;
private:


	UFUNCTION(Server, Reliable)
	void ServerSwapSlots(URpg_ContainerComponent* OtherComponent,
		int32 ThisContainerIndex, int32 ThisSlotIndex,
		int32 OtherContainerIndex, int32 OtherSlotIndex);

	bool InternalSwapSlots(URpg_ContainerComponent* OtherComponent,
		int32 ThisContainerIndex, int32 ThisSlotIndex,
		int32 OtherContainerIndex, int32 OtherSlotIndex);
	
	bool ApplyLocalMappingForTransfer(const FInventoryDragPayload& Payload,
		URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex, int32 TargetSlotIndex);
	
	bool InternalAddItem(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId);
	bool InternalAddItemById(int32 ContainerIndex, const FPrimaryAssetId& ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId);
	bool InternalRemoveItem(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved);
	bool InternalTransferItem(const FInventoryDragPayload& Payload,
	URpg_ContainerComponent* TargetComponent,
	int32 TargetContainerIndex,
	int32 TargetSlotIndex,
	int32& OutMoved);

	int32 FindFirstFreeSlot(int32 ContainerIdx) const;

	UFUNCTION()
	void OnRep_Containers();
};
