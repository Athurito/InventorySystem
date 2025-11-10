// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "RpgInventory/InventoryManagement/Container/InventoryContainerDefinition.h"
#include "UObject/PrimaryAssetId.h"
#include "Rpg_ContainerComponent.generated.h"

struct FInvSlotArray;

USTRUCT(BlueprintType)
struct FInventoryDragPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<class URpg_ContainerComponent> SourceComponent;

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
struct FContainerSlotMap
{
	GENERATED_BODY()
	UPROPERTY() TArray<FGuid> SlotToInstance; // Größe = Rows*Cols, FGuid() == leer
};

// Forward declarations to avoid heavy includes
enum class EInventorySlotType : uint8;
enum class EUseContext : uint8;
class UInventoryContainerDefinition;
class UAbilitySystemComponent;
class APawn;
struct FInventoryFragment_Consumable;
class UInventoryItemComponent;
class UInventoryItemDefinition;

USTRUCT(BlueprintType)
struct FInvContainerEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EInventorySlotType Type = EInventorySlotType::Generic;
	UPROPERTY(BlueprintReadOnly) int32 Index = INDEX_NONE; // Index im Containers-Array
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API URpg_ContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URpg_ContainerComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void AddRepSubObject(UObject* SubObject);

	
protected:
	virtual void BeginPlay() override;

private:
	
};





