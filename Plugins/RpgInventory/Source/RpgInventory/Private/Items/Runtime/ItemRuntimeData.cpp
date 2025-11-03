#include "Items/Runtime/ItemRuntimeData.h"

#include "InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "InventoryManagement/FastArray/Rpg_FastArray.h"

void FItemRuntimeDataContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	if (OwnerComponent && OwnerInstanceId.IsValid())
	{
		const FInv_InventoryEntry Dummy;
		OwnerComponent->OnEntryChanged.Broadcast(OwnerInstanceId, Dummy);
	}
}

void FItemRuntimeDataContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	if (OwnerComponent && OwnerInstanceId.IsValid())
	{
		const FInv_InventoryEntry Dummy;
		OwnerComponent->OnEntryChanged.Broadcast(OwnerInstanceId, Dummy);
	}
}

void FItemRuntimeDataContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	if (OwnerComponent && OwnerInstanceId.IsValid())
	{
		const FInv_InventoryEntry Dummy;
		OwnerComponent->OnEntryChanged.Broadcast(OwnerInstanceId, Dummy);
	}
}

void FItemRuntimeDataContainer::CopyFrom(const FItemRuntimeDataContainer& Src)
{
	Entries = Src.Entries;
	MarkArrayDirty();
	for (FItemRuntimeEntry& E : Entries)
	{
		MarkItemDirty(E);
	}
}

bool FItemRuntimeDataContainer::RemoveByKey(const FGameplayTag& Key)
{
	const int32 Index = Entries.IndexOfByPredicate([&](const FItemRuntimeEntry& E){ return E.Key == Key; });
	if (Index != INDEX_NONE)
	{
		Entries.RemoveAt(Index);
		MarkArrayDirty();
		return true;
	}
	return false;
}
