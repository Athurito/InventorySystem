// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponManagerComponent.generated.h"

class AActor;
class USkeletalMeshComponent;
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

	/**
	 * Initializes the manager with the authoritative inventory manager (PlayerState) and active-item component (Character).
	 * Called by `URpgInventoryWiringComponent` once PlayerState/Inventory are ready on server & clients.
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void InitializeFromInventory(UInventoryManagerComponent* InInventoryManager, UActiveItemComponent* InActiveItemComponent);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	bool bInitialized = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventoryItemInstance>> CurrentHotbar;

	UPROPERTY(Transient)
	TMap<int32, FSpawnedWeaponSlot> SpawnedWeapons;
};
