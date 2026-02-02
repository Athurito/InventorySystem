// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "RpgInventory/InventoryManagement/Container/InventoryContainerDefinition.h"
#include "RpgInventory/InventoryManagement/GAS/InventoryAbilitySet.h"
#include "EquipmentManagerComponent.generated.h"

class UInventoryManagerComponent;
class UInventoryItemInstance;

USTRUCT()
struct FActiveEquipmentSlot
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> AppliedEffects;

	UPROPERTY()
	TObjectPtr<USceneComponent> SpawnedComponent;

	/** Handles for Lyra-style ability sets applied while equipped. */
	UPROPERTY(Transient)
	FInventoryAbilitySetHandles EquipAbilitySetHandles;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API UEquipmentManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentManagerComponent();

	/**
	 * Initializes this manager with the authoritative inventory manager (PlayerState) and the active-item component (Character).
	 * Called by `URpgInventoryWiringComponent` once PlayerState/Inventory are ready on server & clients.
	 */
	UFUNCTION(BlueprintCallable, Category="Equipment")
	void InitializeFromInventory(UInventoryManagerComponent* InInventoryManager, UActiveItemComponent* InActiveItemComponent);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnInventorySlotChanged(int32 ContainerIndex, int32 SlotIndex);

	void RefreshSlot(int32 SlotIndex);

	void UpdateMeshSocket(int32 SlotIndex, UInventoryItemInstance* ItemInstance);

	// Logik für konkrete Slots
	virtual void OnItemEquipped(int32 SlotIndex, UInventoryItemInstance* ItemInstance);
	virtual void OnItemUnequipped(int32 SlotIndex, UInventoryItemInstance* ItemInstance);

public:
	void NotifyActiveSlotChanged(int32 NewActiveSlotIndex);

private:
	UPROPERTY(Transient)
	TObjectPtr<UInventoryManagerComponent> InventoryManager;

	UPROPERTY(Transient)
	class UActiveItemComponent* ActiveItemComponent;

	int32 EquipmentContainerIndex = INDEX_NONE;

	bool bInitialized = false;

	// Speichert die aktuell ausgerüsteten Instanzen für den Vergleich
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventoryItemInstance>> CurrentEquipment;

	UPROPERTY(Transient)
	TMap<int32, FActiveEquipmentSlot> ActiveSlots;
};
