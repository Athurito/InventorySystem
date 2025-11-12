// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RpgInventory/InventoryManagement/Container/InventoryContainerDefinition.h"
#include "RpgInventory/InventoryManagement/FastArray/InventoryItemList.h"
#include "UObject/PrimaryAssetId.h"
#include "Rpg_ContainerComponent.generated.h"

struct FInvSlotArray;

USTRUCT(BlueprintType)
struct FInventoryDragPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<URpg_ContainerComponent> SourceComponent;

	UPROPERTY(BlueprintReadWrite)
	int32 SourceContainerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite)
	int32 SourceSlotIndex = INDEX_NONE;
	
	UPROPERTY(BlueprintReadWrite)
	FGuid InstanceId;

	UPROPERTY(BlueprintReadWrite)
	int32 Quantity = 0; // 0 or less means all
};

USTRUCT()
struct FInventoryContainerInstance
{
	GENERATED_BODY()
	
	UPROPERTY()
	TObjectPtr<const UInventoryContainerDefinition> Definition = nullptr;
	
	UPROPERTY()
	FInventoryList InventoryList;
	
	int32 GetNumSlots() const
	{
		return Definition ? Definition->Rows * Definition->Cols : 0;
	}

	bool IsValidSlot(int32 SlotIndex) const
	{
		return SlotIndex >= 0 && SlotIndex < GetNumSlots();
	}
};

USTRUCT(BlueprintType)
struct FInvContainerEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EInventorySlotType Type = EInventorySlotType::Generic;
	UPROPERTY(BlueprintReadOnly) int32 Index = INDEX_NONE; // Index im Containers-Array
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotChanged, int32, SlotIndex);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API URpg_ContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URpg_ContainerComponent();
	
	void InitializeContainers();
	void AddRepSubObject(UObject* SubObject);

	void BroadcastSlotChanged(int32 SlotIndex) const;

	UInventoryItemInstance* GetItemInstanceInSlot(int32 SlotIndex, int32 ContainerIndex) const;

	UPROPERTY(BlueprintAssignable)
	FOnInventorySlotChanged OnInventorySlotChanged;

protected:
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TArray<TObjectPtr<UInventoryContainerDefinition>> DefaultContainerDefinitions;

	UPROPERTY(Replicated)
	TArray<FInventoryContainerInstance> Containers;
};