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

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void Initialize(UInventoryManagerComponent* InInventoryManager);

protected:
	virtual void BeginPlay() override;

	void OnInventorySlotChanged(int32 ContainerIndex, int32 SlotIndex);

	void EquipItem(FGameplayTag SlotTag, UInventoryItemInstance* ItemInstance);
	void UnequipItem(FGameplayTag SlotTag);

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

private:
	UPROPERTY()
	TObjectPtr<UInventoryManagerComponent> InventoryManager;

	UPROPERTY()
	TArray<FRpgEquipmentEntry> EquipmentEntries;

	// Helper to find which slot a container/slot index refers to
	FGameplayTag GetSlotTagForInventorySlot(int32 ContainerIndex, int32 SlotIndex) const;
};
