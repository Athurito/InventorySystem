// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponManagerComponent.generated.h"

class UInventoryManagerComponent;
class UInventoryItemInstance;
class UActiveItemComponent;

USTRUCT()
struct FSpawnedWeaponSlot
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedActor = nullptr;
};

/**
 * Character-side runtime manager for active weapons/tools.
 * Spawns weapon Actors based on item fragments and attaches them to equip/holster sockets.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API UWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponManagerComponent();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInventorySlotChanged(int32 ContainerIndex, int32 SlotIndex);

	UFUNCTION()
	void OnActiveSlotChanged(int32 NewActiveSlotIndex, int32 OldActiveSlotIndex);

	void RefreshHotbarSlot(int32 SlotIndex);
	void UpdateAttachmentForSlot(int32 SlotIndex);

	AActor* SpawnWeaponActorForItem(int32 SlotIndex, UInventoryItemInstance* Item);
	void DestroyWeaponActorForSlot(int32 SlotIndex);

	USkeletalMeshComponent* GetCharacterMesh() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UInventoryManagerComponent> InventoryManager;

	UPROPERTY(Transient)
	TObjectPtr<UActiveItemComponent> ActiveItemComponent;

	int32 HotbarContainerIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventoryItemInstance>> CurrentHotbar;

	UPROPERTY(Transient)
	TMap<int32, FSpawnedWeaponSlot> SpawnedWeapons;
};
