// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Rpg_ContainerComponent.h"


#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Items/Components/Rpg_ItemComponent.h"
#include "Items/Rpg_ItemDefinition.h"
#include "Items/Fragments/ConsumableFragment.h"
#include "Items/Fragments/StackableFragment.h"
#include "Net/UnrealNetwork.h"
#include "InventoryManagement/Utils/InventoryStatics.h"
#include "InventoryManagement/Use/ItemUseSource_Inventory.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"

URpg_ContainerComponent::URpg_ContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URpg_ContainerComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URpg_ContainerComponent, Containers);
}

void URpg_ContainerComponent::OnRep_Containers()
{
	// Ensure owner is set on containers for client-side delegate forwarding
	for (FInvContainer& C : Containers)
	{
		C.SetOwner(this);
	}
	// Trigger a generic update so UI can rebuild, since FastArray Pre/Post may have missed due to owner pointer
	FInv_InventoryEntry Dummy; // default
	OnItemAdded.Broadcast(Dummy);
}

int32 URpg_ContainerComponent::GetContainerCount() const
{
	return InitialContainerDefs.Num();
}

void URpg_ContainerComponent::TryConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity)
{
	// Backward compatibility: route to unified world-use path
	TryUseWorldItem(ItemComponent, Quantity);
}

void URpg_ContainerComponent::TryUseItemByInstance(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	if (Quantity <= 0) return;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InternalUseItem_Inventory(ContainerIndex, InstanceId, Quantity);
	}
	else
	{
		ServerUseItemByInstance(ContainerIndex, InstanceId, Quantity);
	}
}

void URpg_ContainerComponent::TryUseWorldItem(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	if (!ItemComponent || Quantity <= 0) return;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InternalUseItem_World(ItemComponent, Quantity);
	}
	else
	{
		ServerUseWorldItem(ItemComponent, Quantity);
	}
}

UInventoryContainerDefinition* URpg_ContainerComponent::GetContainerDefinition(const int32 Index) const
{
	if (!InitialContainerDefs.IsValidIndex(Index)) return nullptr;
	
	TSoftObjectPtr<UInventoryContainerDefinition> Def = InitialContainerDefs[Index];
	
	return Def.IsValid() ? Def.Get() : Def.LoadSynchronous();
}

void URpg_ContainerComponent::AddRepSubObject(UObject* SubObject)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObject))
	{
		AddReplicatedSubObject(SubObject);
	}
}

void URpg_ContainerComponent::ServerConsumeItem_Implementation(URpg_ItemComponent* ItemComponent, const int32 Quantity)
{
	if (!ItemComponent || !IsValid(ItemComponent) || !IsValid(ItemComponent->GetOwner())) return;
	// Route to unified world-use path
	ServerUseWorldItem(ItemComponent, Quantity);
}

bool URpg_ContainerComponent::InternalConsume(URpg_ItemComponent* ItemComponent, int32 const Quantity) const
{
	// Legacy path kept for compatibility; route into unified world use
	URpg_ContainerComponent* MutableThis = const_cast<URpg_ContainerComponent*>(this);
	return MutableThis->InternalUseItem_World(ItemComponent, Quantity);
}

void URpg_ContainerComponent::ServerUseItemByInstance_Implementation(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	InternalUseItem_Inventory(ContainerIndex, InstanceId, Quantity);
}

void URpg_ContainerComponent::ServerUseWorldItem_Implementation(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	InternalUseItem_World(ItemComponent, Quantity);
}

static const FConsumableFragment* GetConsumable(const URpg_ItemDefinition* Def)
{
	return Def ? Def->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment) : nullptr;
}

bool URpg_ContainerComponent::InternalUseItem_Inventory(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	if (!(GetOwner() && GetOwner()->HasAuthority())) return false;
	if (Quantity <= 0) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;

	FInv_InventoryEntry* Entry = Containers[ContainerIndex].FindEntryMutableByInstance(InstanceId);
	if (!Entry) return false;
	URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(Entry->GetItemId());
	if (!Def) return false;
	const FConsumableFragment* Cons = GetConsumable(Def);
	if (!Cons) return false;
	
	if (!(Cons && Cons->AllowsContext(EUseContext::Inventory))) return false;

	// Preferred path: event-triggered ability if tag is set (editor/Blueprint-driven)
	if (Cons->UseEventTag.IsValid())
	{
		FGameplayEventData EventData;
		EventData.EventTag = Cons->UseEventTag;
		EventData.Instigator = GetOwner();
		EventData.EventMagnitude = Quantity;
		UObject* SourceObj = UItemUseSource_Inventory::Make(this, this, ContainerIndex, InstanceId);
		EventData.OptionalObject = SourceObj;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), Cons->UseEventTag, EventData);
		OnItemConsumed.Broadcast(nullptr, Quantity);
		return true;
	}
	return false;
}

bool URpg_ContainerComponent::InternalUseItem_World(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	if (!(GetOwner() && GetOwner()->HasAuthority())) return false;
	if (!ItemComponent || Quantity <= 0) return false;
	const URpg_ItemDefinition* Def = ItemComponent->GetItemDefinition();
	if (!Def) return false;
	const FConsumableFragment* Cons = GetConsumable(Def);
	if (!Cons) return false;
	
	if (!(Cons && Cons->AllowsContext(EUseContext::World))) return false;

	// Preferred: event-triggered ability when UseEventTag is configured
	if (Cons->UseEventTag.IsValid())
	{
		FGameplayEventData EventData;
		EventData.EventTag = Cons->UseEventTag;
		EventData.Instigator = GetOwner();
		EventData.Target = ItemComponent->GetOwner();
		EventData.EventMagnitude = Quantity;
		EventData.OptionalObject = ItemComponent; // world item context
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), Cons->UseEventTag, EventData);
		OnItemConsumed.Broadcast(ItemComponent, Quantity);
		return true;
	}

	return false;
}

bool URpg_ContainerComponent::ApplyCostsAndReplicate(FItemRuntimeDataContainer& Runtime, const URpg_ItemDefinition* Def, int32 QuantityPerUse, int32 UsesToApply)
{
	bool bChanged = false;
	if (const FStackableFragment* Stackable = Def->GetFragmentOfTypeWithTag<FStackableFragment>(FragmentTags::StackableFragment))
	{
		if (auto* S = Runtime.FindOrAddMutable<FStackableRuntimeData>(FragmentTags::StackableFragment))
		{
			const int32 Max = FMath::Max(1, Stackable->GetMaxStackSize());
			const int32 Cost = QuantityPerUse * UsesToApply;
			S->CurrentStackCount = FMath::Clamp(S->CurrentStackCount - Cost, 0, Max);
			Runtime.MarkDirty(FragmentTags::StackableFragment);
			bChanged = true;
		}
	}
	return bChanged;
}

void URpg_ContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	Containers.Reset();
	for (const auto& SoftDef : InitialContainerDefs)
	{
		const UInventoryContainerDefinition* Def = SoftDef.IsValid() ? SoftDef.Get() : SoftDef.LoadSynchronous();
		if (!Def) continue;

		FInvContainer C(this);
		C.DisplayName  = Def->DisplayName;
		C.Type         = Def->Type;
		C.Rows         = Def->Rows;
		C.Cols         = Def->Cols;
		C.AllowedItems = Def->AllowedItems;
		C.TabIcon      = Def->TabIcon;
		Containers.Add(MoveTemp(C));
	}
}

bool URpg_ContainerComponent::InternalAddItem(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	OutAdded = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!ItemComponent || Quantity <= 0) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;

	const URpg_ItemDefinition* Def = ItemComponent->GetItemDefinition();
	if (!Def) return false;

	FInvContainer& Cont = Containers[ContainerIndex];
	const FGameplayTag ItemType = Def->GetItemType();
	const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
	const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

	int32 Added = 0;
	FGuid UsedInstance;
	const int32 LastIndex = Cont.AddOrStack(Def->GetPrimaryAssetId(), ItemType, MaxStack, Quantity, UsedInstance, Added);
	OutAdded = Added;
	OutInstanceId = UsedInstance;

	FInv_InventoryEntry* NewEntryPtr = nullptr;
	// If a new stack was created, copy runtime data from the world item component into the new entry
	if (LastIndex != INDEX_NONE)
	{
		NewEntryPtr = Cont.FindEntryMutableByInstance(UsedInstance);
		if (NewEntryPtr)
		{
			NewEntryPtr->CopyRuntimeDataFrom(ItemComponent->GetRuntimeData());
			// Ensure the stack count matches what the container decided for the new stack
			NewEntryPtr->SetStack(NewEntryPtr->GetStack());
		}
	}
	const bool bSuccess = (LastIndex != INDEX_NONE) || (Added > 0);
	if (bSuccess)
	{
		// Broadcast a change (new entry if we have it, otherwise a minimal entry with instance id)
		if (NewEntryPtr)
		{
			OnItemAdded.Broadcast(*NewEntryPtr);
		}
		else
		{
			FInv_InventoryEntry Tmp; Tmp.SetInstanceId(UsedInstance); Tmp.SetItemId(Def->GetPrimaryAssetId());
			OnItemAdded.Broadcast(Tmp);
		}
	}
	return bSuccess;
}

bool URpg_ContainerComponent::InternalRemoveItem(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved)
{
	OutRemoved = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;
	FInvContainer& Cont = Containers[ContainerIndex];
	// Capture a copy before removal for UI update context
	FInv_InventoryEntry* BeforePtr = Cont.FindEntryMutableByInstance(InstanceId);
	FInv_InventoryEntry BeforeCopy;
	if (BeforePtr)
	{
		BeforeCopy = *BeforePtr;
	}
	const bool bRemoved = Cont.RemoveByInstance(InstanceId, Quantity, OutRemoved);
	if (bRemoved && OutRemoved > 0)
	{
		// Prefer to broadcast the remaining entry if it still exists; otherwise broadcast the before copy
		FInv_InventoryEntry* AfterPtr = Cont.FindEntryMutableByInstance(InstanceId);
		if (AfterPtr)
		{
			OnItemRemoved.Broadcast(*AfterPtr);
		}
		else if (BeforePtr)
		{
			OnItemRemoved.Broadcast(BeforeCopy);
		}
		else
		{
			FInv_InventoryEntry Tmp; Tmp.SetInstanceId(InstanceId);
			OnItemRemoved.Broadcast(Tmp);
		}
	}
	return bRemoved;
}

bool URpg_ContainerComponent::InternalAddItemById(int32 ContainerIndex, const FPrimaryAssetId& ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	OutAdded = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (Quantity <= 0) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;
	
	URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(ItemId);

	if (!Def)
	{
		return false; // could not resolve definition from id
	}
	
 	FInvContainer& Cont = Containers[ContainerIndex];
	const FGameplayTag ItemType = Def->GetItemType();
	const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
	const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

	int32 Added = 0;
	FGuid UsedInstance;
	const int32 LastIndex = Cont.AddOrStack(ItemId, ItemType, MaxStack, Quantity, UsedInstance, Added);
	OutAdded = Added;
	OutInstanceId = UsedInstance;
	return LastIndex != INDEX_NONE || Added > 0;
}

bool URpg_ContainerComponent::AddItemToContainerById(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalAddItemById(ContainerIndex, ItemId, Quantity, OutAdded, OutInstanceId);
	}
	else
	{
		ServerAddItemToContainerById(ContainerIndex, ItemId, Quantity);
		OutAdded = 0; // will update via replication
		OutInstanceId.Invalidate();
		return false;
	}
}

bool URpg_ContainerComponent::InternalTransferItem(URpg_ContainerComponent* TargetComponent, int32 SourceContainerIndex, int32 TargetContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutMoved)
{
	OutMoved = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!TargetComponent) return false;
	if (!Containers.IsValidIndex(SourceContainerIndex)) return false;
	if (!TargetComponent->Containers.IsValidIndex(TargetContainerIndex)) return false;

	FInvContainer& Src = Containers[SourceContainerIndex];
	FInvContainer& Dst = TargetComponent->Containers[TargetContainerIndex];

	// Allow intra-component transfers; UI will prevent no-op same-slot. Keep only basic validation below.
	const int32 SrcIdx = Src.FindIndexByInstance(InstanceId);
	if (SrcIdx == INDEX_NONE) return false;
	const FInv_InventoryEntry SrcEntry = Src.GetEntries()[SrcIdx];
	// Respect destination allowed items
	if (!Dst.IsItemAllowed(SrcEntry.GetItemType())) return false;

	int32 Remaining = Quantity;
	if (Remaining <= 0) return false;

	URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(SrcEntry.GetItemId());

	if (!Def)
	{
		return false; // could not resolve definition from id
	}
	
	const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
	const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

	FGuid NewInstanceId = InstanceId;
	int32 Added = 0;
	int32 LastIndex = Dst.AddOrStack(SrcEntry.GetItemId(), SrcEntry.GetItemType(), MaxStack, Remaining, NewInstanceId, Added);
	if (Added <= 0) return false;
	int32 Removed = 0;
	Src.RemoveByInstance(InstanceId, Added, Removed);
	OutMoved = FMath::Min(Added, Removed);
	if (OutMoved > 0)
	{
		// Broadcast to both components for immediate UI updates
		FInv_InventoryEntry* AddedPtr = (LastIndex != INDEX_NONE) ? Dst.FindEntryMutableByInstance(NewInstanceId) : nullptr;
		if (AddedPtr)
		{
			TargetComponent->OnItemAdded.Broadcast(*AddedPtr);
		}
		else
		{
			FInv_InventoryEntry TmpAdd; TmpAdd.SetInstanceId(NewInstanceId); TmpAdd.SetItemId(SrcEntry.GetItemId());
			TargetComponent->OnItemAdded.Broadcast(TmpAdd);
		}

		FInv_InventoryEntry* RemainingPtr = Src.FindEntryMutableByInstance(InstanceId);
		if (RemainingPtr)
		{
			OnItemRemoved.Broadcast(*RemainingPtr);
		}
		else
		{
			OnItemRemoved.Broadcast(SrcEntry);
		}
	}
	return OutMoved > 0;
}

bool URpg_ContainerComponent::AddItemToContainer(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalAddItem(ContainerIndex, ItemComponent, Quantity, OutAdded, OutInstanceId);
	}
	else
	{
		ServerAddItemToContainer(ContainerIndex, ItemComponent, Quantity);
		OutAdded = 0; // will update via replication
		OutInstanceId.Invalidate();
		return false;
	}
}

bool URpg_ContainerComponent::RemoveItemFromContainer(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalRemoveItem(ContainerIndex, InstanceId, Quantity, OutRemoved);
	}
	else
	{
		ServerRemoveItemFromContainer(ContainerIndex, InstanceId, Quantity);
		OutRemoved = 0;
		return false;
	}
}

bool URpg_ContainerComponent::TransferItem(URpg_ContainerComponent* TargetComponent, int32 SourceContainerIndex, int32 TargetContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutMoved)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalTransferItem(TargetComponent, SourceContainerIndex, TargetContainerIndex, InstanceId, Quantity, OutMoved);
	}
	else
	{
		ServerTransferItem(TargetComponent, SourceContainerIndex, TargetContainerIndex, InstanceId, Quantity);
		OutMoved = 0;
		return false;
	}
}

bool URpg_ContainerComponent::GetEntryAtIndex(int32 ContainerIndex, int32 EntryIndex, FInv_InventoryEntry& OutEntry) const
{
	OutEntry = FInv_InventoryEntry();
	if (!Containers.IsValidIndex(ContainerIndex)) return false;
	const FInvContainer& C = Containers[ContainerIndex];
	if (!C.GetEntries().IsValidIndex(EntryIndex)) return false;
	OutEntry = C.GetEntries()[EntryIndex];
	return true;
}

bool URpg_ContainerComponent::FindIndexByInstance(int32 ContainerIndex, const FGuid& InstanceId, int32& OutEntryIndex) const
{
	OutEntryIndex = INDEX_NONE;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;
	OutEntryIndex = Containers[ContainerIndex].FindIndexByInstance(InstanceId);
	return OutEntryIndex != INDEX_NONE;
}

bool URpg_ContainerComponent::CanAcceptFromPayload(int32 TargetContainerIndex, const FInventoryDragPayload& Payload) const
{
	const URpg_ContainerComponent* SrcComp = Payload.SourceComponent.Get();
	if (!SrcComp) return false;
	if (!SrcComp->Containers.IsValidIndex(Payload.SourceContainerIndex)) return false;
	if (!Containers.IsValidIndex(TargetContainerIndex)) return false;

	const FInvContainer& Src = SrcComp->Containers[Payload.SourceContainerIndex];
	const FInvContainer& Dst = Containers[TargetContainerIndex];
	const int32 SrcIdx = Src.FindIndexByInstance(Payload.InstanceId);
	if (SrcIdx == INDEX_NONE) return false;
	const FInv_InventoryEntry& SrcEntry = Src.GetEntries()[SrcIdx];
	return Dst.IsItemAllowed(SrcEntry.GetItemType());
}

bool URpg_ContainerComponent::TransferFromPayloadTo(URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex, const FInventoryDragPayload& Payload, int32& OutMoved)
{
	OutMoved = 0;
	URpg_ContainerComponent* SrcComp = Payload.SourceComponent.Get();
	if (!SrcComp) return false;
	const int32 Quantity = Payload.Quantity > 0 ? Payload.Quantity : INT_MAX/4;
	return SrcComp->TransferItem(TargetComponent, Payload.SourceContainerIndex, TargetContainerIndex, Payload.InstanceId, Quantity, OutMoved);
}

bool URpg_ContainerComponent::SwapSlots(URpg_ContainerComponent* OtherComponent, int32 ThisContainerIndex, int32 ThisSlotIndex, int32 OtherContainerIndex, int32 OtherSlotIndex)
{
	if (!OtherComponent) return false;
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerSwapSlots(OtherComponent, ThisContainerIndex, ThisSlotIndex, OtherContainerIndex, OtherSlotIndex);
		return false;
	}
	if (!Containers.IsValidIndex(ThisContainerIndex)) return false;
	if (!OtherComponent->Containers.IsValidIndex(OtherContainerIndex)) return false;
	FInvContainer& A = Containers[ThisContainerIndex];
	FInvContainer& B = OtherComponent->Containers[OtherContainerIndex];
	if (!A.IsValidEntryIndex(ThisSlotIndex) || !B.IsValidEntryIndex(OtherSlotIndex)) return false;

	// No-op: same comp and same slot
	if (OtherComponent == this && ThisContainerIndex == OtherContainerIndex && ThisSlotIndex == OtherSlotIndex) return false;

	FInv_InventoryEntry* EntryA = A.GetEntryMutableByIndex(ThisSlotIndex);
	FInv_InventoryEntry* EntryB = B.GetEntryMutableByIndex(OtherSlotIndex);
	const bool bAEmpty = !EntryA || EntryA->GetInstanceId().IsValid() == false;
	const bool bBEmpty = !EntryB || EntryB->GetInstanceId().IsValid() == false;

	// If one side empty: move the other
	if (bAEmpty ^ bBEmpty)
	{
		URpg_ContainerComponent* SrcComp = bBEmpty ? this : OtherComponent;
		URpg_ContainerComponent* DstComp = bBEmpty ? OtherComponent : this;
		const int32 SrcContainerIdx = bBEmpty ? ThisContainerIndex : OtherContainerIndex;
		const int32 DstContainerIdx = bBEmpty ? OtherContainerIndex : ThisContainerIndex;
		FInv_InventoryEntry* MoveEntry = bBEmpty ? EntryA : EntryB;
		if (!MoveEntry) return false;
		int32 Moved = 0;
		return SrcComp->InternalTransferItem(DstComp, SrcContainerIdx, DstContainerIdx, MoveEntry->GetInstanceId(), MoveEntry->GetStack(), Moved);
	}

	// Both have items: check allowed types for each destination
	if (!B.IsItemAllowed(EntryA->GetItemType()) || !A.IsItemAllowed(EntryB->GetItemType()))
	{
		return false;
	}
	// Perform swap
	const bool bSwapped = A.SwapEntriesByIndex(ThisSlotIndex, B, OtherSlotIndex);
	if (!bSwapped) return false;

	// Broadcast updates for UI
	if (A.IsValidEntryIndex(ThisSlotIndex))
	{
		OnItemRemoved.Broadcast(A.GetEntries()[ThisSlotIndex]);
		OnItemAdded.Broadcast(A.GetEntries()[ThisSlotIndex]);
	}
	if (B.IsValidEntryIndex(OtherSlotIndex))
	{
		OtherComponent->OnItemRemoved.Broadcast(B.GetEntries()[OtherSlotIndex]);
		OtherComponent->OnItemAdded.Broadcast(B.GetEntries()[OtherSlotIndex]);
	}
	return true;
}

void URpg_ContainerComponent::ServerSwapSlots_Implementation(URpg_ContainerComponent* OtherComponent, int32 ThisContainerIndex, int32 ThisSlotIndex, int32 OtherContainerIndex, int32 OtherSlotIndex)
{
	SwapSlots(OtherComponent, ThisContainerIndex, ThisSlotIndex, OtherContainerIndex, OtherSlotIndex);
}

bool URpg_ContainerComponent::AutoDepositMatchingTo(URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex, int32& OutTotalMoved)
{
	OutTotalMoved = 0;

	// Route to server if not authoritative
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerAutoDepositMatchingTo(TargetComponent, TargetContainerIndex);
		return false; // UI will update via replication callbacks
	}

	if (!TargetComponent) return false;
	if (!TargetComponent->Containers.IsValidIndex(TargetContainerIndex)) return false;

	FInvContainer& Dst = TargetComponent->Containers[TargetContainerIndex];

	// 1) Collect item ids that already exist in destination
	TSet<FPrimaryAssetId> ExistingIds;
	for (const FInv_InventoryEntry& E : Dst.GetEntries())
	{
		ExistingIds.Add(E.GetItemId());
	}
	if (ExistingIds.Num() == 0)
	{
		// Nothing in destination — don't introduce new types
		return false;
	}

	// 2) Iterate all our source containers
	for (int32 SrcIdx = 0; SrcIdx < Containers.Num(); ++SrcIdx)
	{
		if (!Containers.IsValidIndex(SrcIdx)) continue;
		FInvContainer& Src = Containers[SrcIdx];

		// Optional rule: within same component block moves between identical types
		if (TargetComponent == this && Src.Type == Dst.Type)
		{
			continue;
		}

		// Copy candidate instance ids up-front to avoid iterator invalidation on remove
		TArray<FGuid> CandidateInstances;
		CandidateInstances.Reserve(Src.GetEntries().Num());
		for (const FInv_InventoryEntry& E : Src.GetEntries())
		{
			if (ExistingIds.Contains(E.GetItemId()))
			{
				CandidateInstances.Add(E.GetInstanceId());
			}
		}

		for (const FGuid& InstanceId : CandidateInstances)
		{
			const int32 Index = Src.FindIndexByInstance(InstanceId);
			if (Index == INDEX_NONE) continue;
			const FInv_InventoryEntry SrcEntry = Src.GetEntries()[Index];

			// Respect destination allowed items
			if (!Dst.IsItemAllowed(SrcEntry.GetItemType())) continue;

			// Resolve MaxStack from definition
			URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(SrcEntry.GetItemId());
			if (!Def) continue;
			const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
			const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

			int32 Remaining = SrcEntry.GetStack();
			if (Remaining <= 0) continue;

			FGuid NewInstanceId = InstanceId;
			int32 Added = 0;
			Dst.AddOrStack(SrcEntry.GetItemId(), SrcEntry.GetItemType(), MaxStack, Remaining, NewInstanceId, Added);
			if (Added <= 0) continue;

			int32 Removed = 0;
			Src.RemoveByInstance(InstanceId, Added, Removed);
			OutTotalMoved += FMath::Min(Added, Removed);
		}
	}

	return OutTotalMoved > 0;
}

void URpg_ContainerComponent::ServerAddItemToContainer_Implementation(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	int32 DummyAdded; FGuid DummyId; InternalAddItem(ContainerIndex, ItemComponent, Quantity, DummyAdded, DummyId);
}

void URpg_ContainerComponent::ServerRemoveItemFromContainer_Implementation(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	int32 DummyRemoved; InternalRemoveItem(ContainerIndex, InstanceId, Quantity, DummyRemoved);
}

void URpg_ContainerComponent::ServerAddItemToContainerById_Implementation(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity)
{
	int32 DummyAdded; FGuid DummyId; InternalAddItemById(ContainerIndex, ItemId, Quantity, DummyAdded, DummyId);
}

void URpg_ContainerComponent::ServerTransferItem_Implementation(URpg_ContainerComponent* TargetComponent, int32 SourceContainerIndex, int32 TargetContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	int32 Dummy; InternalTransferItem(TargetComponent, SourceContainerIndex, TargetContainerIndex, InstanceId, Quantity, Dummy);
}

void URpg_ContainerComponent::ServerAutoDepositMatchingTo_Implementation(URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex)
{
	int32 DummyMoved = 0;
	AutoDepositMatchingTo(TargetComponent, TargetContainerIndex, DummyMoved);
}
