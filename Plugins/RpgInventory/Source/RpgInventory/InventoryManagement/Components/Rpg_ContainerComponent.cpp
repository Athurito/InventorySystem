// Fill out your copyright notice in the Description page of Project Settings.

#include "Rpg_ContainerComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

URpg_ContainerComponent::URpg_ContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URpg_ContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URpg_ContainerComponent, Containers);
}

void URpg_ContainerComponent::InitializeContainers()
{
	Containers.Empty();

	for (UInventoryContainerDefinition* Def : DefaultContainerDefinitions)
	{
		FInventoryContainerInstance& NewInstance = Containers.Emplace_GetRef();
		NewInstance.Definition = Def;
		NewInstance.InventoryList.OwnerComponent = this; // falls du das brauchst
	}
}


void URpg_ContainerComponent::AddRepSubObject(UObject* SubObject)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObject))
	{
		AddReplicatedSubObject(SubObject);
	}
}

void URpg_ContainerComponent::BroadcastSlotChanged(int32 SlotIndex) const
{
	OnInventorySlotChanged.Broadcast(SlotIndex);
}

UInventoryItemInstance* URpg_ContainerComponent::GetItemInstanceInSlot(int32 SlotIndex, int32 ContainerIndex) const
{
	if (Containers.IsEmpty() || !Containers.IsValidIndex(ContainerIndex) || SlotIndex < 0)
	{
		return nullptr;
	}
	
	return Containers[ContainerIndex].InventoryList.GetItemInstanceInSlot(SlotIndex);
}

void URpg_ContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
}

