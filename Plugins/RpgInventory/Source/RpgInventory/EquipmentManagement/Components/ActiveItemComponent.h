// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "RpgInventory/InventoryManagement/GAS/InventoryAbilitySet.h"
#include "ActiveItemComponent.generated.h"

class UInventoryManagerComponent;
class UInventoryItemInstance;
class UEquipmentManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveEquipmentSlotChanged, int32, NewActiveSlotIndex, int32, OldActiveSlotIndex);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API UActiveItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActiveItemComponent();

	/** Fired on server when active slot changes and on clients via OnRep. */
	UPROPERTY(BlueprintAssignable, Category="ActiveItem")
	FOnActiveEquipmentSlotChanged OnActiveEquipmentSlotChanged;

	UFUNCTION(BlueprintCallable, Category = "ActiveItem")
	void SetActiveSlot(int32 EquipmentSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "ActiveItem")
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UInventoryManagerComponent> InventoryManager;

	UPROPERTY(Transient)
	TObjectPtr<UEquipmentManagerComponent> EquipmentManager;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = INDEX_NONE;

	UFUNCTION()
	void OnRep_ActiveSlotIndex(int32 OldIndex);

	void UpdateActiveVisuals();
	void BroadcastActiveSlotChanged(int32 OldIndex);
	void ApplyActiveGrants(UInventoryItemInstance* Item);
	void RemoveActiveGrants();

	UPROPERTY(Transient)
	FInventoryAbilitySetHandles ActiveAbilitySetHandles;

	// Legacy direct ability grants (kept for backwards compatibility)
	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> LegacyGrantedAbilityHandles;
};
