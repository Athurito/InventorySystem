// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryManagerComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemDefinition.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"

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

	for (UInventoryContainerDefinition* Def : DefaultContainerDefinitions)
	{
		FInventoryContainerInstance& NewInstance = Containers.Emplace_GetRef();
		NewInstance.Definition = Def;
		NewInstance.InventoryList.OwnerComponent = this; // falls du das brauchst
	}
}


void UInventoryManagerComponent::AddRepSubObject(UObject* SubObject)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObject))
	{
		AddReplicatedSubObject(SubObject);
	}
}

void UInventoryManagerComponent::BroadcastSlotChanged(int32 SlotIndex) const
{
	OnInventorySlotChanged.Broadcast(SlotIndex);
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

void UInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	InitializeContainers();
}

