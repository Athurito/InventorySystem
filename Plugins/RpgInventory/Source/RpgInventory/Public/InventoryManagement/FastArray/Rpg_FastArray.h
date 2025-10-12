// // Fill out your copyright notice in the Description page of Project Settings.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "GameplayTagContainer.h"
// #include "Net/Serialization/FastArraySerializer.h"
//
// class URpg_InventoryComponent;
// /**
//  * 
//  */
// /** A single entry in an inventory */
// USTRUCT(BlueprintType)
// struct FInv_InventoryEntry : public FFastArraySerializerItem
// {
// 	GENERATED_BODY()
// 	
// 	FInv_InventoryEntry() {  }
//
// private:
// 	friend struct FInv_InventoryFastArray;
// 	friend URpg_InventoryComponent;
// 	
// 	UPROPERTY()
// 	TObjectPtr<UInv_InventoryItem> Item = nullptr;
// 	
// };
//
// /** A single entry in an inventory */
// USTRUCT(BlueprintType)
// struct FInv_InventoryFastArray : public FFastArraySerializer
// {
// 	GENERATED_BODY()
//
// 	FInv_InventoryFastArray() : OwnerComponent(nullptr) { }
// 	FInv_InventoryFastArray(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) { }
//
// 	TArray<UInv_InventoryItem*> GetAllItems() const;
//
// 	// FFastArraySerializer contract
// 	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
// 	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
// 	// End of FFastArraySerializer contract
//
// 	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParam)
// 	{
// 		return FastArrayDeltaSerialize<FInv_InventoryEntry, FInv_InventoryFastArray>(Entries, DeltaParam, *this);
// 	}
//
// 	UInv_InventoryItem* AddEntry(UInv_ItemComponent* ItemComponent);
// 	UInv_InventoryItem* AddEntry(UInv_InventoryItem* Item);
// 	void RemoveEntry(UInv_InventoryItem* Item);
//
// 	UInv_InventoryItem* FindFirstItemByType(const FGameplayTag& ItemType) const;
//
// private:
// 	friend UInv_InventoryComponent;
// 	
// 	//Replicated list of items
// 	UPROPERTY()
// 	TArray<FInv_InventoryEntry> Entries;
// 	
// 	UPROPERTY(NotReplicated)
// 	TObjectPtr<UActorComponent> OwnerComponent;
// };
//
// template<>
// struct TStructOpsTypeTraits<FInv_InventoryFastArray> : TStructOpsTypeTraitsBase2<FInv_InventoryFastArray>
// {
// 	enum  { WithNetDeltaSerializer = true };
// };
