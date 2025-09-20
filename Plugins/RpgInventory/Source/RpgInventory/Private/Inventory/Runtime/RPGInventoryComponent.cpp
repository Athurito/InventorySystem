#include "Inventory/Runtime/RPGInventoryComponent.h"
#include "Inventory/ItemSystem/ItemDefinition.h"
#include "Net/UnrealNetwork.h"

URPGInventoryComponent::URPGInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	Items.Owner = this;
}

void URPGInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool URPGInventoryComponent::HasFreeSlot() const
{
	// Find any slot index in [0, Capacity) that is not used by an entry
	for (int32 Slot = 0; Slot < Capacity; ++Slot)
	{
		if (FindEntryIndexBySlot(Slot) == INDEX_NONE)
		{
			return true;
		}
	}
	return false;
}

bool URPGInventoryComponent::AddItem(UItemDefinition* Definition, int32 StackCount)
{
	if (!ensure(Definition) || StackCount <= 0)
	{
		return false;
	}
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false; // server-only in this minimal implementation
	}

	// Find first free slot
	int32 FreeSlot = INDEX_NONE;
	for (int32 Slot = 0; Slot < Capacity; ++Slot)
	{
		if (FindEntryIndexBySlot(Slot) == INDEX_NONE)
		{
			FreeSlot = Slot;
			break;
		}
	}
	if (FreeSlot == INDEX_NONE)
	{
		return false; // no space
	}

	FInventoryEntry Entry;
	Entry.EntryId = NextId++;
	Entry.Definition = Definition;
	Entry.StackCount = StackCount;
	Entry.SlotIndex = FreeSlot;
	Items.Entries.Add(Entry);

	// mark dirty for fast array
	Items.MarkItemDirty(Items.Entries.Last());
	return true;
}

bool URPGInventoryComponent::RemoveByEntryId(int32 EntryId, int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	for (int32 i=0;i<Items.Entries.Num();++i)
	{
		if (Items.Entries[i].EntryId == EntryId)
		{
			// For simplicity, remove whole entry
			Items.Entries.RemoveAt(i);
			Items.MarkArrayDirty();
			return true;
		}
	}
	return false;
}

int32 URPGInventoryComponent::GetEntryIdAtSlot(int32 Slot) const
{
	if (Slot < 0 || Slot >= Capacity) return INDEX_NONE;
	const int32 Idx = FindEntryIndexBySlot(Slot);
	return (Idx != INDEX_NONE) ? Items.Entries[Idx].EntryId : INDEX_NONE;
}

bool URPGInventoryComponent::MoveSlotToSlot(int32 FromSlot, int32 ToSlot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (FromSlot == ToSlot) return true;
	if (FromSlot < 0 || ToSlot < 0 || FromSlot >= Capacity || ToSlot >= Capacity) return false;

	const int32 FromIdx = FindEntryIndexBySlot(FromSlot);
	if (FromIdx == INDEX_NONE) return false;
	const int32 ToIdx = FindEntryIndexBySlot(ToSlot);

	if (ToIdx == INDEX_NONE)
	{
		// Move into empty slot
		Items.Entries[FromIdx].SlotIndex = ToSlot;
		Items.MarkItemDirty(Items.Entries[FromIdx]);
		return true;
	}
	else
	{
		// Swap
		Items.Entries[FromIdx].SlotIndex = ToSlot;
		Items.Entries[ToIdx].SlotIndex = FromSlot;
		Items.MarkItemDirty(Items.Entries[FromIdx]);
		Items.MarkItemDirty(Items.Entries[ToIdx]);
		return true;
	}
}

int32 URPGInventoryComponent::FindEntryIndexBySlot(int32 Slot) const
{
	for (int32 i = 0; i < Items.Entries.Num(); ++i)
	{
		if (Items.Entries[i].SlotIndex == Slot)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void URPGInventoryComponent::OnRep_Items()
{
	OnInventoryChanged.Broadcast();
}

void URPGInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URPGInventoryComponent, Items);
}

void URPGInventoryComponent::Server_MoveSlotToSlot_Implementation(int32 FromSlot, int32 ToSlot)
{
	MoveSlotToSlot(FromSlot, ToSlot);
}
