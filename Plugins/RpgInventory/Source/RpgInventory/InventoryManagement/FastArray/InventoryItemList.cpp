// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemList.h"


FString FInventoryEntry::GetDebugString() const
{
	return FString();
}

TArray<UInventoryItemInstance*> FInventoryList::GetAllItems() const
{
	return TArray<UInventoryItemInstance*>();
}

void FInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
}

UInventoryItemInstance* FInventoryList::AddEntry(TSubclassOf<UInventoryItemDefinition> ItemClass, int32 StackCount)
{
	return nullptr;
}

void FInventoryList::AddEntry(UInventoryItemInstance* Instance)
{
}

void FInventoryList::RemoveEntry(UInventoryItemInstance* Instance)
{
}
