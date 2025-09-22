// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionManagement/Interface/Interactable.h"
#include "InventoryManagement/Rpg_InteractableBaseComponent.h"
#include "Items/ItemData.h"
#include "Rpg_ItemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API URpg_ItemComponent : public URpg_InteractableBaseComponent
{
	GENERATED_BODY()

public:
	URpg_ItemComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitItemData(UItemData CopyOfItemData);
	TObjectPtr<UItemData> GetItemData() { return ItemData; }

	/* IInteractable start*/
	virtual FInteractDisplayData GetDisplayData_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Instigator) const override;
	virtual void Interact_Implementation(APawn* Instigator) override;
	/* IInteractable end*/

protected:
	virtual void BeginPlay() override;

private:

	UPROPERTY(Replicated, EditAnywhere, Category = "Inventory")
	TObjectPtr<UItemData> ItemData = nullptr;
};
