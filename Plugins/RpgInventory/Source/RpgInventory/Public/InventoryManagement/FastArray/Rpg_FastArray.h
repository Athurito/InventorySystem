// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InventoryManagement/Container/InventoryContainerDefinition.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Items/Runtime/ItemRuntimeData.h"
#include "Items/Fragments/Rpg_FragmentTags.h"
#include "Rpg_FastArray.generated.h"

class URpg_ItemComponent;
class URpg_ContainerComponent;

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
	
	int32 GetStack() const {
		if (const FStackableRuntimeData* D = RuntimeData.FindConst<FStackableRuntimeData>(FragmentTags::StackableFragment))
		{
			return D->CurrentStackCount;
		}
		// Default wenn kein StackableRuntimeData existiert
		return 1;
	}
	void SetStack(const int32 NewStack);
	void SetRuntimeDataOwner(URpg_ContainerComponent* NewOwner)
	{
		RuntimeData.OwnerComponent = NewOwner;
		RuntimeData.OwnerInstanceId = InstanceId;
	}

	FGameplayTag GetItemType() const {return ItemType;}
	void SetItemType(const FGameplayTag& NewItemType) {ItemType = NewItemType;}

	// Access to runtime data (for container/component bridging)
	const FItemRuntimeDataContainer& GetRuntimeData() const { return RuntimeData; }
	FItemRuntimeDataContainer& GetRuntimeDataMutable() { return RuntimeData; }
	void CopyRuntimeDataFrom(const FItemRuntimeDataContainer& Src) { RuntimeData.CopyFrom(Src); }
	
	bool CanStackWith(const FInv_InventoryEntry& Other, const int32 MaxStackParam) const { return ItemId == Other.ItemId && GetStack() < MaxStackParam; }
private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FPrimaryAssetId ItemId;      
	UPROPERTY() FItemRuntimeDataContainer RuntimeData;
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
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	// End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParam)
	{
		return FastArrayDeltaSerialize<FInv_InventoryEntry, FInvContainer>(Entries, DeltaParam, *this);
	}

public:
	void SetOwner(UActorComponent* InOwner) { OwnerComponent = InOwner; }
	int32 GetNum() const { return Entries.Num(); }
	const TArray<FInv_InventoryEntry>& GetEntries() const{ return Entries; }
	TArray<FInv_InventoryEntry>& GetEntries(){ return Entries; }
	bool IsItemAllowed(const FGameplayTag& ItemTag) const;
	int32 FindIndexByInstance(const FGuid& InstanceId) const;
	FInv_InventoryEntry* FindEntryMutableByInstance(const FGuid& InstanceId);
	bool IsValidEntryIndex(int32 Index) const { return Entries.IsValidIndex(Index); }
	FInv_InventoryEntry* GetEntryMutableByIndex(int32 Index);
	bool SwapEntriesByIndex(int32 IndexA, FInvContainer& Other, int32 IndexB);
	int32 AddOrStack(const FPrimaryAssetId& ItemId, const FGameplayTag& ItemType, int32 MaxStack, int32 Quantity, FGuid& OutInstanceId, int32& OutAdded);
	bool RemoveByInstance(const FGuid& InstanceId, int32 Quantity, int32& OutRemoved);
	bool StackIntoIndex(int32 Index, int32 MaxStack, int32 Quantity, int32& OutAdded);

	int32 AddNewStackExact(const FPrimaryAssetId& ItemId, const FGameplayTag& ItemType,
									  int32 Quantity, FGuid& OutInstanceId);

	bool SplitIntoNewEntry(const FGuid& SourceInstanceId, int32 SplitQty, FGuid& OutNewInstanceId);

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
