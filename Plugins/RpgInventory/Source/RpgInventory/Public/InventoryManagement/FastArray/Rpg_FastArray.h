// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InventoryManagement/Container/InventoryContainerDefinition.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Rpg_FastArray.generated.h"

class URpg_ItemComponent;
class URpg_ContainerComponent;
/**
 * 
 */

UENUM(BlueprintType)
enum class EInv_ItemCategory : uint8 { Generic, Consumable, Quest, Equipment };
/** A single entry in an inventory */
USTRUCT(BlueprintType)
struct FInv_InventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	FInv_InventoryEntry() {  }

	FGuid GetInstanceId() const {return InstanceId;}
	void SetInstanceId(const FGuid NewInstanceId) {InstanceId = NewInstanceId;}
	
	FPrimaryAssetId GetItemId() const {return ItemId;}
	void SetItemId(const FPrimaryAssetId& NewItemId) {ItemId = NewItemId;}
	
	int32 GetStack() const {return Stack;}
	void SetStack(const int32 NewStack) {Stack = NewStack;}

	FGameplayTag GetItemType() const {return ItemType;}
	void SetItemType(const FGameplayTag& NewItemType) {ItemType = NewItemType;}
	
	bool IsStackable() const;
	bool IsConsumable() const;
private:
	UPROPERTY() FGuid           InstanceId;  
	UPROPERTY() FPrimaryAssetId ItemId;      
	UPROPERTY() int32           Stack = 1;
	FGameplayTag ItemType = FGameplayTag::EmptyTag;
	
};

USTRUCT(BlueprintType)
struct FInvContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FInvContainer() : OwnerComponent(nullptr) { }
	explicit FInvContainer(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) { }

	// --- Meta (nicht/dauerhaft, selten geändert) ---
	UPROPERTY(BlueprintReadOnly) FText  DisplayName;
	UPROPERTY(BlueprintReadOnly) EInventorySlotType Type = EInventorySlotType::Generic;
	UPROPERTY(BlueprintReadOnly) int32 Rows = 5;
	UPROPERTY(BlueprintReadOnly) int32 Cols = 6;
	UPROPERTY() FGameplayTagQuery AllowedItems; // für Add-Validierung
	UPROPERTY() TObjectPtr<UTexture2D> TabIcon = nullptr;
	
	// FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	// End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParam)
	{
		return FastArrayDeltaSerialize<FInv_InventoryEntry, FInvContainer>(Entries, DeltaParam, *this);
	}

private:
	//Replicated list of items
	UPROPERTY()
	TArray<FInv_InventoryEntry> Entries;
	
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FInvContainer> : TStructOpsTypeTraitsBase2<FInvContainer>
{
	enum  { WithNetDeltaSerializer = true };
};
