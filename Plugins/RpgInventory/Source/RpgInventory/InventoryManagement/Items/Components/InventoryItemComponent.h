// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RpgInventory/InteractionManagement/Data/InteractableDataAsset.h"
#include "RpgInventory/InventoryManagement/Rpg_InteractableBaseComponent.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemDefinition.h"
#include "InventoryItemComponent.generated.h"

class UInventoryItemDefinition;
class APawn;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API UInventoryItemComponent : public URpg_InteractableBaseComponent
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category="Inventory|Init")
	void InitItemByDefinition(UInventoryItemDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category="Inventory|Init")
	void InitItemById(FPrimaryAssetId Id);

	UFUNCTION(BlueprintCallable, Category="Inventory|Init")
	void InitItemBySoft(TSoftObjectPtr<UInventoryItemDefinition> Soft);
	
	const UInventoryItemDefinition* GetItemDefinition() const { return ItemDefinition.Get(); }
	
	int32 GetMaxStackSize() const;
	
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
	
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1"))
	int32 InitialStackCount = 1;

	UPROPERTY(ReplicatedUsing=OnRep_ItemId)
	FPrimaryAssetId ItemId;

	UPROPERTY(Replicated)
	int32 CurrentStackCount = 1;
	
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TSoftObjectPtr<UInventoryItemDefinition> ItemDefinition;
	
	UFUNCTION()
	void OnRep_ItemId();
};
