// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionManagement/Interface/Interactable.h"
#include "InventoryManagement/Rpg_InteractableBaseComponent.h"
#include "Items/Rpg_ItemDefinition.h"
#include "Rpg_ItemComponent.generated.h"

class APawn;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API URpg_ItemComponent : public URpg_InteractableBaseComponent
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;



	// ---- Initialisierung (nur Server aufrufen) ----
	UFUNCTION(BlueprintCallable, Category="Inventory|Init")
	void InitItemByDefinition(URpg_ItemDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category="Inventory|Init")
	void InitItemById(FPrimaryAssetId Id);

	UFUNCTION(BlueprintCallable, Category="Inventory|Init")
	void InitItemBySoft(TSoftObjectPtr<URpg_ItemDefinition> Soft);

	UPROPERTY(EditInstanceOnly,BlueprintReadOnly, Category="Inventory", meta=(ExposeOnSpawn))
	TSoftObjectPtr<URpg_ItemDefinition> InitialDefinition;
	
	const URpg_ItemDefinition* GetItemDefinition() const { return ItemDefinition.Get(); }

	// Runtime stack for this instance (replicated), initialized from ItemData's StackableFragment once
	int32 GetCurrentStackCount() const { return CurrentStackCount; }
	int32 GetMaxStackSize() const { return MaxStackSize; }

	// Attempts to consume this item according to its Consumable Fragment rules
	bool Consume(APawn* Instigator);

	/* IInteractable start*/
	virtual FInteractDisplayData GetDisplayData_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Instigator) const override;
	virtual void Interact_Implementation(APawn* Instigator) override;
	/* IInteractable end*/

protected:
	virtual void BeginPlay() override;

private:
	
	UPROPERTY(ReplicatedUsing=OnRep_ItemId)
	FPrimaryAssetId ItemId;
	
	UPROPERTY(Transient)
	TSoftObjectPtr<URpg_ItemDefinition> ItemDefinition;
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentStackCount)
	int32 CurrentStackCount = 1;
	
	int32 MaxStackSize = 1;

	UFUNCTION()
	void OnRep_ItemId();

	UFUNCTION()
	void OnRep_CurrentStackCount();

	void InitRuntimeFromDefinition(const URpg_ItemDefinition* Def);

};
