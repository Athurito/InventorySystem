// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "RpgEquipmentManagerComponent.generated.h"

class UInventoryItemInstance;
class URpgEquipmentInstance;
class UInventoryManagerComponent;
class URpgEquipmentDefinition;
class UAbilitySystemComponent;

USTRUCT()
struct FRpgEquipmentEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag SlotTag;

	UPROPERTY()
	TObjectPtr<UInventoryItemInstance> ItemInstance;

	UPROPERTY()
	TObjectPtr<URpgEquipmentInstance> EquipmentInstance;
};

/**
 * Manages equipment for an actor. Listens to an InventoryManagerComponent.
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API URpgEquipmentManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URpgEquipmentManagerComponent();
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void Initialize(UInventoryManagerComponent* InInventoryManager);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetActiveHotbarSlot(int32 ContainerIndex, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UseActiveItem();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnInventorySlotChanged(int32 ContainerIndex, int32 SlotIndex);

	void EquipItem(FGameplayTag SlotTag, UInventoryItemInstance* ItemInstance, bool bIsDynamic = false);
	void UnequipItem(FGameplayTag SlotTag);

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

private:
	UPROPERTY()
	TObjectPtr<UInventoryManagerComponent> InventoryManager;

	UPROPERTY(Replicated)
	TArray<FRpgEquipmentEntry> EquipmentEntries;

	// New: Track active hotbar slot
	int32 ActiveHotbarContainerIndex = INDEX_NONE;
	int32 ActiveHotbarSlotIndex = INDEX_NONE;

	// Helper to find which slot a container/slot index refers to
	FGameplayTag GetSlotTagForInventorySlot(int32 ContainerIndex, int32 SlotIndex) const;
};
