// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagerComponent.h"

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
}

void UInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryManagerComponent, Containers);
}

void UInventoryManagerComponent::InitializeContainers()
{
    Containers.Empty();

    for (int32 DefIdx = 0; DefIdx < DefaultContainerDefinitions.Num(); ++DefIdx)
    {
        UInventoryContainerDefinition* Def = DefaultContainerDefinitions[DefIdx];
        FInventoryContainerInstance& NewInstance = Containers.Emplace_GetRef();
        NewInstance.Definition = Def;
        NewInstance.InventoryList.OwnerComponent = this; // falls du das brauchst
        // ContainerIndex für Events/Broadcasts setzen (nicht repliziert)
        NewInstance.InventoryList.ContainerIndex = DefIdx;
    }
}


void UInventoryManagerComponent::AddRepSubObject(UObject* SubObject)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObject))
	{
		AddReplicatedSubObject(SubObject);
	}
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
	if (Containers.IsEmpty() || !Containers.IsValidIndex(ContainerIndex) || SlotIndex < 0)
	{
		return nullptr;
	}
	
	return Containers[ContainerIndex].InventoryList.GetItemInstanceInSlot(SlotIndex);
}

UInventoryItemInstance* UInventoryManagerComponent::AddItemDefinition(UInventoryItemDefinition* ItemDefinition, int32 SlotIndex, int32 ContainerIndex, int32 StackCount)
{
	UInventoryItemInstance* Result = nullptr;
	if (ItemDefinition != nullptr)
	{
		check(Containers.IsValidIndex(ContainerIndex));
		Result = Containers[ContainerIndex].InventoryList.AddEntry(ItemDefinition, SlotIndex, StackCount);
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
		{
			AddReplicatedSubObject(Result);
		}
	}
	return Result;
}

void UInventoryManagerComponent::AddItemInstance(UInventoryItemInstance* Instance, int32 SlotIndex, int32 ContainerIndex)
{
	Containers[ContainerIndex].InventoryList.AddEntry(Instance, SlotIndex);
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Instance)
	{
		AddReplicatedSubObject(Instance);
	}
}

void UInventoryManagerComponent::RemoveItemInstance(UInventoryItemInstance* Instance, int32 ContainerIndex)
{
	Containers[ContainerIndex].InventoryList.RemoveEntry(Instance);
	if (Instance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(Instance);
	}
}

TArray<UInventoryItemInstance*> UInventoryManagerComponent::GetAllItems(int32 ContainerIndex) const
{
    if (Containers.IsEmpty() || !Containers.IsValidIndex(ContainerIndex))
        return TArray<UInventoryItemInstance*>();
    
    return Containers[ContainerIndex].InventoryList.GetAllItems();
}

void UInventoryManagerComponent::SetInventoryClickAction(EInventoryClickAction Action)
{
	InventoryClickAction = Action;
}

EInventoryClickAction UInventoryManagerComponent::GetInventoryClickAction() const
{
	return InventoryClickAction;
}

void UInventoryManagerComponent::HandleDrop(UInventoryManagerComponent* SourceManager, int32 SourceContainerIndex,
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

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	InitializeContainers();
}

void UInventoryManagerComponent::OnRep_Containers()
{
	for (int32 i = 0; i < Containers.Num(); ++i)
	{
		Containers[i].InventoryList.OwnerComponent = this;
		Containers[i].InventoryList.ContainerIndex = i;
	}
}

void UInventoryManagerComponent::HandleDropSameContainer(int32 ContainerIndex, int32 SourceSlotIndex,
                                                         int32 DestSlotIndex, int32 DragQuantity, FGameplayTag OperationType)
{
	if (SourceSlotIndex == DestSlotIndex)
	{
		return; // nichts tun
	}

	FInventoryList& List = GetInventoryList(ContainerIndex);

	UInventoryItemInstance* SourceItem = List.GetItemInstanceInSlot(SourceSlotIndex);
	UInventoryItemInstance* DestItem   = List.GetItemInstanceInSlot(DestSlotIndex);

	if (!SourceItem)
	{
		return;
	}

	// 1) Ziel leer → Move oder Split
	if (!DestItem)
	{
		if (OperationType.MatchesTagExact(RpgTags::InventoryOperation_SplitOperation) && DragQuantity > 0 && DragQuantity < SourceItem->GetStatTagStackCount(FragmentTags::StackableFragment))
		{
			SplitStack(List, SourceSlotIndex, DestSlotIndex, DragQuantity);
		}
		else
		{
			MoveItem(List, SourceSlotIndex, DestSlotIndex);
		}
		return;
	}

	// 2) Ziel belegt → Stack oder Swap
	if (CanStack(SourceItem, DestItem))
	{
		MergeStacks(List, SourceSlotIndex, DestSlotIndex, DragQuantity);
	}
	else
	{
		SwapItems(List, SourceSlotIndex, DestSlotIndex);
	}
}

void UInventoryManagerComponent::HandleDropDifferentContainer(UInventoryManagerComponent* SourceManager,
	int32 SourceContainerIndex, int32 SourceSlotIndex, int32 TargetContainerIndex, int32 TargetSlotIndex,
	int32 DragQuantity, FGameplayTag OperationType)
{
	if (!SourceManager) return;

	FInventoryList& SourceList = SourceManager->GetInventoryList(SourceContainerIndex);
	FInventoryList& DestList   = GetInventoryList(TargetContainerIndex);

	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceSlotIndex);
	UInventoryItemInstance* DestItem   = DestList.GetItemInstanceInSlot(TargetSlotIndex);

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
	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceSlotIndex);
	if (!SourceItem)
	{
		return;
	}

	// SourceList: Eintrag entfernen
	SourceList.RemoveEntry(SourceItem);

	// Rep: Aus SourceManager austragen
	if (SourceManager->IsUsingRegisteredSubObjectList())
	{
		SourceManager->RemoveReplicatedSubObject(SourceItem);
	}

	// DestList: Instanz im Zielslot hinzufügen
	DestList.AddEntry(SourceItem, DestSlotIndex);

	// Rep: beim Zielmanager registrieren
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(SourceItem);
	}

	// UI informieren
	// SourceManager->BroadcastSlotChanged(SourceContainerIndex, SourceSlotIndex);
	// BroadcastSlotChanged(DestContainerIndex, DestSlotIndex);
}

void UInventoryManagerComponent::SplitStackAcrossContainers(UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList, int32 SourceContainerIndex, int32 SourceSlotIndex, FInventoryList& DestList,
	int32 DestContainerIndex, int32 DestSlotIndex, int32 SplitQuantity)
{
	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceSlotIndex);
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

	// Neue Instanz im Zielcontainer, zunächst mit 0 Stack
	UInventoryItemInstance* NewInstance = DestList.AddEntry(Def, DestSlotIndex, /*InitialStack*/ 0);

	// Stackwerte ABSOLUT setzen
	
	SourceItem->SetStatTagStackCount(FragmentTags::StackableFragment, SourceCount - SplitQuantity);
	NewInstance->SetStatTagStackCount(FragmentTags::StackableFragment, SplitQuantity);

	// Rep: neue Instanz beim Zielmanager registrieren
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && NewInstance)
	{
		AddReplicatedSubObject(NewInstance);
	}

	// UI informieren
	// SourceManager->BroadcastSlotChanged(SourceContainerIndex, SourceSlotIndex);
	// BroadcastSlotChanged(DestContainerIndex, DestSlotIndex);
}

void UInventoryManagerComponent::MergeStacksAcrossContainers(UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList, int32 SourceContainerIndex, int32 SourceSlotIndex, FInventoryList& DestList,
	int32 DestContainerIndex, int32 DestSlotIndex, int32 DragQuantity)
{
	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceSlotIndex);
	UInventoryItemInstance* TargetItem   = DestList.GetItemInstanceInSlot(DestSlotIndex);

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
		if (SourceManager->IsUsingRegisteredSubObjectList())
		{
			SourceManager->RemoveReplicatedSubObject(SourceItem);
		}
	}

	// SourceManager->BroadcastSlotChanged(SourceContainerIndex, SourceSlotIndex);
	// BroadcastSlotChanged(DestContainerIndex, DestSlotIndex);
}

void UInventoryManagerComponent::SwapItemsAcrossContainers(UInventoryManagerComponent* SourceManager,
	FInventoryList& SourceList, int32 SourceContainerIndex, int32 SourceSlotIndex, FInventoryList& DestList,
	int32 DestContainerIndex, int32 DestSlotIndex)
{
	UInventoryItemInstance* SourceItem = SourceList.GetItemInstanceInSlot(SourceSlotIndex);
	UInventoryItemInstance* DestItem   = DestList.GetItemInstanceInSlot(DestSlotIndex);

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

	// Rep: Subobjects abmelden
	if (SourceItem && SourceManager->IsUsingRegisteredSubObjectList())
	{
		SourceManager->RemoveReplicatedSubObject(SourceItem);
	}
	if (DestItem && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(DestItem);
	}

	// Einfügen in jeweils andere Liste
	if (SourceItem)
	{
		DestList.AddEntry(SourceItem, DestSlotIndex);
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
		{
			AddReplicatedSubObject(SourceItem);
		}
	}

	if (DestItem)
	{
		SourceList.AddEntry(DestItem, SourceSlotIndex);
		if (SourceManager->IsUsingRegisteredSubObjectList() && SourceManager->IsReadyForReplication())
		{
			SourceManager->AddReplicatedSubObject(DestItem);
		}
	}

	// UI-Updates
	// SourceManager->BroadcastSlotChanged(SourceContainerIndex, SourceSlotIndex);
	// BroadcastSlotChanged(DestContainerIndex, DestSlotIndex);
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

void UInventoryManagerComponent::MoveItem(FInventoryList& List, int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (SourceSlotIndex == TargetSlotIndex)
	{
		return;
	}

	UInventoryItemInstance* Instance = List.GetItemInstanceInSlot(SourceSlotIndex);
	if (!Instance)
	{
		return;
	}

	// Einfach SlotIndex auf dem Entry ändern – du brauchst hier Zugriff auf Entry
	// Am saubersten: Hilfsfunktion in FInventoryList schreiben:

	List.MoveEntry(SourceSlotIndex, TargetSlotIndex);

	// UI informieren:
	// BroadcastSlotChanged(List.ContainerIndex, SourceSlotIndex);
	// BroadcastSlotChanged(List.ContainerIndex, TargetSlotIndex);
}

void UInventoryManagerComponent::SwapItems(FInventoryList& List, int32 SlotA, int32 SlotB)
{
	if (SlotA == SlotB)
	{
		return;
	}

	FInventoryEntry* EntryA = nullptr;
	FInventoryEntry* EntryB = nullptr;

	for (FInventoryEntry& Entry : List.Entries)
	{
		if (Entry.SlotIndex == SlotA) EntryA = &Entry;
		else if (Entry.SlotIndex == SlotB) EntryB = &Entry;
	}

	if (!EntryA && !EntryB)
	{
		return;
	}
	if (EntryA) { EntryA->SlotIndex = SlotB; List.MarkItemDirty(*EntryA); }
	if (EntryB) { EntryB->SlotIndex = SlotA; List.MarkItemDirty(*EntryB); }

	// BroadcastSlotChanged(List.ContainerIndex, SlotA);
	// BroadcastSlotChanged(List.ContainerIndex, SlotB);
}

void UInventoryManagerComponent::MergeStacks(FInventoryList& List, int32 SourceSlotIndex, int32 TargetSlotIndex,
	int32 DragQuantity)
{
	UInventoryItemInstance* SourceItem = List.GetItemInstanceInSlot(SourceSlotIndex);
	UInventoryItemInstance* DestItem   = List.GetItemInstanceInSlot(TargetSlotIndex);
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

	// BroadcastSlotChanged(List.ContainerIndex, SourceSlotIndex);
	// BroadcastSlotChanged(List.ContainerIndex, TargetSlotIndex);
}

void UInventoryManagerComponent::SplitStack(FInventoryList& List, int32 SourceSlotIndex, int32 TargetSlotIndex,
	int32 SplitQuantity)
{
	UInventoryItemInstance* SourceItem = List.GetItemInstanceInSlot(SourceSlotIndex);
	if (!SourceItem) return;

	const int32 SourceCount = SourceItem->GetStatTagStackCount( FragmentTags::StackableFragment);
	if (SplitQuantity <= 0 || SplitQuantity >= SourceCount) return;

	// Neue Instance mit gleicher Def
	UInventoryItemDefinition* Def = SourceItem->GetItemDefinition();
	UInventoryItemInstance* NewInstance = List.AddEntry(Def, TargetSlotIndex, SplitQuantity);
	//SourceItem->AddStatTagStack(FragmentTags::StackableFragment, SourceCount - SplitQuantity);
	
	
	SourceItem->SetStatTagStackCount( FragmentTags::StackableFragment, SourceCount - SplitQuantity);
	//SetStackCount(SourceItem, SourceCount - SplitQuantity);

	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && NewInstance)
	{
		AddReplicatedSubObject(NewInstance);
	}

	// BroadcastSlotChanged(List.ContainerIndex, SourceSlotIndex);
	// BroadcastSlotChanged(List.ContainerIndex, TargetSlotIndex);
}

bool UInventoryManagerComponent::CanPlaceInContainer(UInventoryItemInstance* SourceItem, int32 TargetContainerIndex,
	int32 TargetSlotIndex) const
{
	if (!SourceItem) return false;
	
	return true;
}

const FInventoryList& UInventoryManagerComponent::GetInventoryList(int32 ContainerIndex) const
{
	check(Containers.IsValidIndex(ContainerIndex));
	return Containers[ContainerIndex].InventoryList;
}

FInventoryList& UInventoryManagerComponent::GetInventoryList(int32 ContainerIndex)
{
	check(Containers.IsValidIndex(ContainerIndex));
	return Containers[ContainerIndex].InventoryList;
}

