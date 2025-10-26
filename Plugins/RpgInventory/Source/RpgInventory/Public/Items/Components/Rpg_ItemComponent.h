// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionManagement/Interface/Interactable.h"
#include "InventoryManagement/Rpg_InteractableBaseComponent.h"
#include "Items/Rpg_ItemDefinition.h"
#include "GameplayTagContainer.h"
#include "Items/Runtime/ItemRuntimeData.h"
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

	// Runtime stack access via RuntimeData (single source of truth)
	int32 GetCurrentStackCount() const;
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

public:
	// Runtime data access helpers keyed by fragment tag
	template<typename T>
	T* GetMutableFragmentData(const FGameplayTag& Key)
	{
		return RuntimeData.FindMutable<T>(Key);
	}

	template<typename T>
	T* GetOrCreateMutableFragmentData(const FGameplayTag& Key)
	{
		return RuntimeData.FindOrAddMutable<T>(Key);
	}

private:
	
	UPROPERTY(ReplicatedUsing=OnRep_ItemId)
	FPrimaryAssetId ItemId;
	
	UPROPERTY(Transient)
	TSoftObjectPtr<URpg_ItemDefinition> ItemDefinition;

	// Modular runtime data container (delta replicated)
	UPROPERTY(ReplicatedUsing=OnRep_RuntimeData)
	FItemRuntimeDataContainer RuntimeData;

	UFUNCTION()
	void OnRep_ItemId();

	UFUNCTION()
	void OnRep_RuntimeData();
	
	void InitRuntimeFromDefinition(const URpg_ItemDefinition* Def);
		
};
