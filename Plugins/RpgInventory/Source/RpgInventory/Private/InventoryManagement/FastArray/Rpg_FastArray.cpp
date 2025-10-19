// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagement/FastArray/Rpg_FastArray.h"

#include "InventoryManagement/Components/Rpg_InventoryComponent.h"
#include "Items/Rpg_InventoryItem.h"
#include "Items/Components/Rpg_ItemComponent.h"

TArray<URpg_InventoryItem*> FInv_InventoryFastArray::GetAllItems() const
{
	TArray<URpg_InventoryItem*> Items;
	Items.Reserve(Entries.Num());

	for (const auto& Entry : Entries)
	{
		if (!Entry.Item)
			continue;
		
		Items.Add(Entry.Item);
	}
	return Items;
}

void FInv_InventoryFastArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	const TObjectPtr<URpg_InventoryComponent> InventoryComponent = Cast<URpg_InventoryComponent>(OwnerComponent);

	if (!InventoryComponent)
	{
		return;
	}

	for (int32 const Index : RemovedIndices)
	{
		InventoryComponent->OnItemRemoved.Broadcast(Entries[Index].Item);
	}
}

void FInv_InventoryFastArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	const TObjectPtr<URpg_InventoryComponent> InventoryComponent = Cast<URpg_InventoryComponent>(OwnerComponent);

	if (!InventoryComponent)
	{
		return;
	}

	for (int32 const Index : AddedIndices)
	{
		InventoryComponent->OnItemRemoved.Broadcast(Entries[Index].Item);
	}
}

URpg_InventoryItem* FInv_InventoryFastArray::AddEntry(URpg_ItemComponent* ItemComponent)
{
	check(OwnerComponent)
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority())

	URpg_InventoryComponent* InventoryComponent = Cast<URpg_InventoryComponent>(OwnerComponent);

	if (!IsValid(InventoryComponent))
	{
		return nullptr;
	}
	
	FInv_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	// NewEntry.Item = ItemComponent->GetItemManifest().Manifest(OwningActor);
	InventoryComponent->AddRepSubObject(NewEntry.Item);

	MarkItemDirty(NewEntry);

	return NewEntry.Item;
}


URpg_InventoryItem* FInv_InventoryFastArray::AddEntry(URpg_InventoryItem* Item)
{
	check(OwnerComponent)
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority())

	FInv_InventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Item = Item;

	MarkItemDirty(NewEntry);

	return Item;
}


void FInv_InventoryFastArray::RemoveEntry(URpg_InventoryItem* Item)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FInv_InventoryEntry& Entry = *EntryIt;
		
		if (EntryIt->Item == Item)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

URpg_InventoryItem* FInv_InventoryFastArray::FindFirstItemByType(const FGameplayTag& ItemType) const
{
	// auto* FoundItem = Entries.FindByPredicate([ItemType](const FInv_InventoryEntry& Entry)
	// {
	// 	return IsValid(Entry.Item) && Entry.Item->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
	// });
	//
	// return FoundItem ? FoundItem->Item : nullptr;

	return nullptr;
}
