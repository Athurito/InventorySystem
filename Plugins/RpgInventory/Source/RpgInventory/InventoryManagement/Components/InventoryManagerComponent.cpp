// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagerComponent.h"

#include "Engine/ActorChannel.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "RpgInventory/InventoryManagement/GameplayTags/InventoryOperationTags.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Stackable.h"

UInventoryManagerComponent::UInventoryManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	InventoryList.OwnerComponent = this;
}

bool UInventoryManagerComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	// Alle Item-Instanzen aus der InventoryList replizieren
	for (FInventoryEntry& Entry : InventoryList.Entries)
	{
		if (Entry.Instance)
		{
			bWroteSomething |= Channel->ReplicateSubobject(Entry.Instance, *Bunch, *RepFlags);
		}
	}

	return bWroteSomething;
}

void UInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryManagerComponent, InventoryList);
}

void UInventoryManagerComponent::BroadcastSlotChanged(int32 ContainerIndex, int32 SlotIndex) const
{
	const bool bAuth = GetOwner() && GetOwner()->HasAuthority();
	UE_LOG(LogTemp, Warning, TEXT("SlotChanged C=%d S=%d  Auth=%d"),
		ContainerIndex, SlotIndex, bAuth ? 1 : 0);

    OnInventorySlotChanged.Broadcast(ContainerIndex, SlotIndex);
}

UInventoryItemInstance* UInventoryManagerComponent::GetItemInstanceInSlot(int32 SlotIndex, int32 ContainerIndex) const
{
	return InventoryList.GetItemInstanceInSlot(ContainerIndex, SlotIndex);
}

int32 UInventoryManagerComponent::GetNumSlots(int32 ContainerIndex) const
{
	if (!DefaultContainerDefinitions.IsValidIndex(ContainerIndex) || !DefaultContainerDefinitions[ContainerIndex])
	{
		return 0;
	}
	return DefaultContainerDefinitions[ContainerIndex]->TotalSlots;
}

UInventoryItemInstance* UInventoryManagerComponent::AddItemDefinition(UInventoryItemDefinition* ItemDefinition, int32 SlotIndex, int32 ContainerIndex, int32 StackCount)
{
	UInventoryItemInstance* Result = nullptr;
	if (ItemDefinition)
	{
		Result = InventoryList.AddEntry(ItemDefinition, ContainerIndex, SlotIndex, StackCount);
	}
	return Result;
}

void UInventoryManagerComponent::AddItemInstance(UInventoryItemInstance* Instance, int32 SlotIndex, int32 ContainerIndex)
{
	InventoryList.AddEntry(Instance, ContainerIndex, SlotIndex);
}

void UInventoryManagerComponent::RemoveItemInstance(UInventoryItemInstance* Instance)
{
	InventoryList.RemoveEntry(Instance);
}

TArray<UInventoryItemInstance*> UInventoryManagerComponent::GetAllItems(int32 ContainerIndex) const
{
	return InventoryList.GetAllItemsInContainer(ContainerIndex);
}

void UInventoryManagerComponent::SetInventoryClickAction(EInventoryClickAction Action)
{
	InventoryClickAction = Action;
}

EInventoryClickAction UInventoryManagerComponent::GetInventoryClickAction() const
{
	return InventoryClickAction;
}

void UInventoryManagerComponent::HandleDrop_Internal(UInventoryManagerComponent* SourceManager, int32 SourceContainerIndex,
                                            int32 SourceSlotIndex, int32 TargetContainerIndex, int32 TargetSlotIndex, int32 DragQuantity,
                                            FGameplayTag OperationType)
{
	if (!SourceManager)
	{
		return;
	}

	// 1) Gleicher Manager & Container?
	const bool bSameManager   = (SourceManager == this);
	const bool bSameContainer = bSameManager && (SourceContainerIndex == TargetContainerIndex);

	if (bSameContainer)
	{
		HandleDropSameContainer(SourceContainerIndex, SourceSlotIndex, TargetSlotIndex, DragQuantity, OperationType);
	}
	else
	{
		HandleDropDifferentContainer(SourceManager, SourceContainerIndex, SourceSlotIndex, TargetContainerIndex, TargetSlotIndex, DragQuantity, OperationType);
	}
}

void UInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	InventoryList.OwnerComponent = this;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
}

void UInventoryManagerComponent::HandleDropSameContainer(int32 ContainerIndex, int32 SourceSlotIndex,
                                                         int32 DestSlotIndex, int32 DragQuantity, FGameplayTag OperationType)
{
	if (SourceSlotIndex == DestSlotIndex)
	{
		return; // nichts tun
	}

	FInventoryList& List = InventoryList;

	UInventoryItemInstance* SourceItem = List.GetItemInstanceInSlot(ContainerIndex, SourceSlotIndex);
	UInventoryItemInstance* DestItem   = List.GetItemInstanceInSlot(ContainerIndex, DestSlotIndex);

	if (!SourceItem)
	{
		return;
	}

	// 1) Ziel leer → Move oder Split
	if (!DestItem)
	{
		if (OperationType.MatchesTagExact(RpgTags::InventoryOperation_SplitOperation) && DragQuantity > 0 && DragQuantity < SourceItem->GetStatTagStackCount(FragmentTags::StackableFragment))
		{
			SplitStack(List,ContainerIndex, SourceSlotIndex, DestSlotIndex, DragQuantity);
		}
		else
		{
			MoveItem(List, ContainerIndex, SourceSlotIndex, DestSlotIndex);
		}
		return;
	}

	// 2) Ziel belegt → Stack oder Swap
	if (CanStack(SourceItem, DestItem))
	{
		MergeStacks(List, ContainerIndex, SourceSlotIndex, DestSlotIndex, DragQuantity);
	}
	else
	{
		SwapItems(List, ContainerIndex, SourceSlotIndex, DestSlotIndex);
	}
}

void UInventoryManagerComponent::HandleDropDifferentContainer(UInventoryManagerComponent* SourceManager,
	int32 SourceContainerIndex, int32 SourceSlotIndex, int32 TargetContainerIndex, int32 TargetSlotIndex,
	int32 DragQuantity, FGameplayTag OperationType)
{
	if (!SourceManager) return;

	FInventoryList& SourceList = SourceManager->GetInventoryList();
	FInventoryList& DestList   = GetInventoryList();

	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceContainerIndex, SourceSlotIndex);
	UInventoryItemInstance* DestItem   = DestList.GetItemInstanceInSlot(TargetContainerIndex, TargetSlotIndex);

	if (!SourceItem)
	{
		return;
	}

	//Optional: TagQuery / ContainerDefinition prüfen
	if (!CanPlaceInContainer(SourceItem, TargetContainerIndex, TargetSlotIndex)) return;

	//1) Ziel leer → Move / Split über zwei Listen
	if (!DestItem)
	{
		if (OperationType.MatchesTagExact(RpgTags::InventoryOperation_SplitOperation) && DragQuantity > 0 && DragQuantity < SourceItem->GetStatTagStackCount(FragmentTags::StackableFragment))
		{
			SplitStackAcrossContainers(SourceManager, SourceList, SourceContainerIndex, SourceSlotIndex,
									   DestList, TargetContainerIndex, TargetSlotIndex, DragQuantity);
		}
		else
		{
			MoveItemAcrossContainers(SourceManager, SourceList, SourceContainerIndex, SourceSlotIndex,
									 DestList, TargetContainerIndex, TargetSlotIndex);
		}
		return;
	}
	
	// 2) Ziel belegt → Stack oder Swap über zwei Container
	if (CanStack(SourceItem, DestItem))
	{
		MergeStacksAcrossContainers(SourceManager, SourceList, SourceContainerIndex, SourceSlotIndex,
									DestList, TargetContainerIndex, TargetSlotIndex, DragQuantity);
	}
	else
	{
		SwapItemsAcrossContainers(SourceManager, SourceList, SourceContainerIndex, SourceSlotIndex,
								  DestList, TargetContainerIndex, TargetSlotIndex);
	}
}

void UInventoryManagerComponent::MoveItemAcrossContainers(UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList, int32 SourceContainerIndex, int32 SourceSlotIndex, FInventoryList& DestList,
	int32 DestContainerIndex, int32 DestSlotIndex)
{
	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceContainerIndex, SourceSlotIndex);
	if (!SourceItem)
	{
		return;
	}

	// SourceList: Eintrag entfernen
	SourceList.RemoveEntry(SourceItem);
	
	// DestList: Instanz im Zielslot hinzufügen
	DestList.AddEntry(SourceItem, DestContainerIndex,  DestSlotIndex);
	
	// UI informieren
	SourceManager->BroadcastSlotChanged(SourceContainerIndex, SourceSlotIndex);
	BroadcastSlotChanged(DestContainerIndex, DestSlotIndex);
}

void UInventoryManagerComponent::SplitStackAcrossContainers(UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList, int32 SourceContainerIndex, int32 SourceSlotIndex, FInventoryList& DestList,
	int32 DestContainerIndex, int32 DestSlotIndex, int32 SplitQuantity)
{
	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceContainerIndex, SourceSlotIndex);
	if (!SourceItem)
	{
		return;
	}

	const int32 SourceCount = SourceItem->GetStatTagStackCount(FragmentTags::StackableFragment);
	if (SplitQuantity <= 0 || SplitQuantity >= SourceCount)
	{
		return;
	}

	UInventoryItemDefinition* Def = SourceItem->GetItemDefinition();
	UInventoryItemInstance* NewInstance = DestList.AddEntry(Def, DestContainerIndex, DestSlotIndex, /*InitialStack*/ 0);
	
	SourceItem->SetStatTagStackCount(FragmentTags::StackableFragment, SourceCount - SplitQuantity);
	NewInstance->SetStatTagStackCount(FragmentTags::StackableFragment, SplitQuantity);
	

	// UI informieren
	SourceManager->BroadcastSlotChanged(SourceContainerIndex, SourceSlotIndex);
	BroadcastSlotChanged(DestContainerIndex, DestSlotIndex);
}

void UInventoryManagerComponent::MergeStacksAcrossContainers(UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList, int32 SourceContainerIndex, int32 SourceSlotIndex, FInventoryList& DestList,
	int32 DestContainerIndex, int32 DestSlotIndex, int32 DragQuantity)
{
	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceContainerIndex, SourceSlotIndex);
	UInventoryItemInstance* TargetItem   = DestList.GetItemInstanceInSlot(DestContainerIndex, DestSlotIndex);

	if (!SourceItem || !TargetItem)
	{
		return;
	}

	const UInventoryFragment_Stackable* StackFrag = SourceItem->FindFragmentByClass<UInventoryFragment_Stackable>();
	if (!StackFrag)
	{
		return;
	}

	const int32 MaxStack = StackFrag->GetMaxStackSize();

	int32 SourceCount = SourceItem->GetStatTagStackCount( FragmentTags::StackableFragment);
	int32 TargetCount   = TargetItem->GetStatTagStackCount( FragmentTags::StackableFragment);

	if (SourceCount <= 0)
	{
		return;
	}

	int32 AmountToMove = (DragQuantity > 0) ? FMath::Min(DragQuantity, SourceCount) : SourceCount;
	int32 FreeSpace    = MaxStack - TargetCount;

	if (FreeSpace <= 0)
	{
		return;
	}

	const int32 ActuallyMoved = FMath::Min(AmountToMove, FreeSpace);
	SourceItem->SetStatTagStackCount(FragmentTags::StackableFragment, SourceCount - ActuallyMoved);
	TargetItem->SetStatTagStackCount(FragmentTags::StackableFragment, TargetCount + ActuallyMoved);

	// Wenn Source leer, Eintrag & Rep entfernen
	if (SourceItem->GetStatTagStackCount(FragmentTags::StackableFragment) <= 0)
	{
		SourceList.RemoveEntry(SourceItem);
	}

	SourceManager->BroadcastSlotChanged(SourceContainerIndex, SourceSlotIndex);
	BroadcastSlotChanged(DestContainerIndex, DestSlotIndex);
}

void UInventoryManagerComponent::SwapItemsAcrossContainers(UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList, int32 SourceContainerIndex, int32 SourceSlotIndex, FInventoryList& DestList,
	int32 DestContainerIndex, int32 DestSlotIndex)
{
	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceContainerIndex, SourceSlotIndex);
	UInventoryItemInstance* DestItem   = DestList.GetItemInstanceInSlot(DestContainerIndex, DestSlotIndex);

	if (!SourceItem && !DestItem)
	{
		return;
	}

	// Entfernen aus aktuellen Listen
	if (SourceItem)
	{
		SourceList.RemoveEntry(SourceItem);
	}
	if (DestItem)
	{
		DestList.RemoveEntry(DestItem);
	}

	// Einfügen in jeweils andere Liste
	if (SourceItem)
	{
		DestList.AddEntry(SourceItem, DestContainerIndex, DestSlotIndex);
	}

	if (DestItem)
	{
		SourceList.AddEntry(DestItem, SourceContainerIndex, SourceSlotIndex);
	}

	// UI-Updates
	SourceManager->BroadcastSlotChanged(SourceContainerIndex, SourceSlotIndex);
	BroadcastSlotChanged(DestContainerIndex, DestSlotIndex);
}

bool UInventoryManagerComponent::CanStack(UInventoryItemInstance* A, UInventoryItemInstance* B) const
{
	if (!A || !B)
	{
		return false;
	}

	// gleiche ItemDef?
	if (A->GetItemDefinition() != B->GetItemDefinition())
	{
		return false;
	}

	// Stackable-Fragment?
	const UInventoryFragment_Stackable* StackFragA = A->FindFragmentByClass<UInventoryFragment_Stackable>();
	const UInventoryFragment_Stackable* StackFragB = B->FindFragmentByClass<UInventoryFragment_Stackable>();

	if (!StackFragA || !StackFragB)
	{
		return false;
	}

	const int32 MaxStack = StackFragA->GetMaxStackSize();
	const int32 CountA   = A->GetStatTagStackCount( FragmentTags::StackableFragment);
	const int32 CountB   = B->GetStatTagStackCount( FragmentTags::StackableFragment);

	return (CountA < MaxStack || CountB < MaxStack);
}

void UInventoryManagerComponent::MoveItem(FInventoryList& List, int32 ContainerIndex, int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (SourceSlotIndex == TargetSlotIndex)
	{
		return;
	}

	UInventoryItemInstance* Instance = List.GetItemInstanceInSlot(ContainerIndex, SourceSlotIndex);
	if (!Instance)
	{
		return;
	}

	List.MoveEntry(ContainerIndex, SourceSlotIndex, TargetSlotIndex);

	// UI informieren:
	BroadcastSlotChanged(ContainerIndex, SourceSlotIndex);
	BroadcastSlotChanged(ContainerIndex, TargetSlotIndex);
}

void UInventoryManagerComponent::SwapItems(FInventoryList& List, int32 ContainerIndex, int32 SlotA, int32 SlotB)
{
	if (SlotA == SlotB)
	{
		return;
	}

	FInventoryEntry* EntryA = nullptr;
	FInventoryEntry* EntryB = nullptr;

	for (FInventoryEntry& Entry : List.Entries)
	{
		if (Entry.ContainerIndex != ContainerIndex)
		{
			continue;
		}

		if (Entry.SlotIndex == SlotA) EntryA = &Entry;
		else if (Entry.SlotIndex == SlotB) EntryB = &Entry;
	}

	if (!EntryA && !EntryB)
	{
		return;
	}
	if (EntryA) { EntryA->SlotIndex = SlotB; List.MarkItemDirty(*EntryA); }
	if (EntryB) { EntryB->SlotIndex = SlotA; List.MarkItemDirty(*EntryB); }

	BroadcastSlotChanged(ContainerIndex, SlotA);
	BroadcastSlotChanged(ContainerIndex, SlotB);
}

void UInventoryManagerComponent::MergeStacks(FInventoryList& List,int32 ContainerIndex, int32 SourceSlotIndex, int32 TargetSlotIndex,
	int32 DragQuantity)
{
	UInventoryItemInstance* SourceItem = List.GetItemInstanceInSlot(ContainerIndex, SourceSlotIndex);
	UInventoryItemInstance* DestItem   = List.GetItemInstanceInSlot(ContainerIndex, TargetSlotIndex);
	if (!SourceItem || !DestItem) return;

	const UInventoryFragment_Stackable* StackFrag = SourceItem->FindFragmentByClass<UInventoryFragment_Stackable>();
	if (!StackFrag) return;

	const int32 MaxStack = StackFrag->GetMaxStackSize();
	int32 SourceCount = SourceItem->GetStatTagStackCount( FragmentTags::StackableFragment);
	int32 DestCount   = DestItem->GetStatTagStackCount( FragmentTags::StackableFragment);

	if (SourceCount <= 0) return;

	int32 AmountToMove = DragQuantity > 0 ? FMath::Min(DragQuantity, SourceCount) : SourceCount;
	int32 FreeSpace    = MaxStack - DestCount;

	if (FreeSpace <= 0)
	{
		return;
	}

	const int32 ActuallyMoved = FMath::Min(AmountToMove, FreeSpace);

	SourceItem->SetStatTagStackCount(FragmentTags::StackableFragment, SourceCount - ActuallyMoved);
	DestItem->SetStatTagStackCount(FragmentTags::StackableFragment, DestCount + ActuallyMoved);

	// Wenn Source leer geworden ist, Eintrag löschen
	
	if (SourceItem->GetStatTagStackCount( FragmentTags::StackableFragment) <= 0)
	{
		List.RemoveEntry(SourceItem);
	}

	BroadcastSlotChanged(ContainerIndex, SourceSlotIndex);
	BroadcastSlotChanged(ContainerIndex, TargetSlotIndex);
}

void UInventoryManagerComponent::SplitStack(FInventoryList& List, int32 ContainerIndex, int32 SourceSlotIndex, int32 TargetSlotIndex,
	int32 SplitQuantity)
{
	UInventoryItemInstance* SourceItem = List.GetItemInstanceInSlot(ContainerIndex, SourceSlotIndex);
	if (!SourceItem) return;

	const int32 SourceCount = SourceItem->GetStatTagStackCount( FragmentTags::StackableFragment);
	if (SplitQuantity <= 0 || SplitQuantity >= SourceCount) return;

	// Neue Instance mit gleicher Def
	UInventoryItemDefinition* Def = SourceItem->GetItemDefinition();
	UInventoryItemInstance* NewInstance = List.AddEntry(Def, ContainerIndex, TargetSlotIndex, SplitQuantity);
	
	SourceItem->SetStatTagStackCount( FragmentTags::StackableFragment, SourceCount - SplitQuantity);

	BroadcastSlotChanged(ContainerIndex, SourceSlotIndex);
	BroadcastSlotChanged(ContainerIndex, TargetSlotIndex);
}

bool UInventoryManagerComponent::CanPlaceInContainer(UInventoryItemInstance* SourceItem, int32 TargetContainerIndex,
	int32 TargetSlotIndex) const
{
	if (!SourceItem) return false;
	
	return true;
}

