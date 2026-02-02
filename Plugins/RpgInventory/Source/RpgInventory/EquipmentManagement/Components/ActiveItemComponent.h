// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RpgInventory/InventoryManagement/GAS/InventoryAbilitySet.h"
#include "ActiveItemComponent.generated.h"

class UInventoryManagerComponent;
class UInventoryItemInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveHotbarSlotChanged, int32, NewActiveHotbarSlotIndex, int32, OldActiveHotbarSlotIndex);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName = "Active Item Component"))
class RPGINVENTORY_API UActiveItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActiveItemComponent();

	/**
	 * Initializes this component with the authoritative inventory manager (typically on PlayerState).
	 * Called by `URpgInventoryWiringComponent` once PlayerState/Inventory are ready on server & clients.
	 */
	UFUNCTION(BlueprintCallable, Category = "ActiveItem")
	void InitializeFromInventory(UInventoryManagerComponent* InInventoryManager);

	/** Fired on server when active slot changes and on clients via OnRep. */
	UPROPERTY(BlueprintAssignable, Category="ActiveItem")
	FOnActiveHotbarSlotChanged OnActiveHotbarSlotChanged;

	UFUNCTION(BlueprintCallable, Category = "ActiveItem")
	void SetActiveSlot(int32 HotbarSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "ActiveItem")
	int32 GetActiveSlotIndex() const { return ActiveHotbarSlotIndex; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UInventoryManagerComponent> InventoryManager;

	int32 HotbarContainerIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveHotbarSlotIndex)
	int32 ActiveHotbarSlotIndex = INDEX_NONE;

	UFUNCTION()
	void OnRep_ActiveHotbarSlotIndex(int32 OldIndex);

	void BroadcastActiveSlotChanged(int32 OldIndex);
	void ApplyActiveGrants(UInventoryItemInstance* Item);
	void RemoveActiveGrants();

	UPROPERTY(Transient)
	FInventoryAbilitySetHandles ActiveAbilitySetHandles;
};
