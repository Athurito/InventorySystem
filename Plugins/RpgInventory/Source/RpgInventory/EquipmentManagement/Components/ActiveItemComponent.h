// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveItemComponent.generated.h"

class UInventoryManagerComponent;
class UInventoryItemInstance;
class UEquipmentManagerComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API UActiveItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActiveItemComponent();

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
	void GrantAbilities(UInventoryItemInstance* Item);
	void RemoveAbilities();

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};
