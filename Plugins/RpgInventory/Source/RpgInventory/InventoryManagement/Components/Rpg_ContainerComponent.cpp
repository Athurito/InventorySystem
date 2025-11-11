// Fill out your copyright notice in the Description page of Project Settings.

#include "Rpg_ContainerComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

URpg_ContainerComponent::URpg_ContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URpg_ContainerComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// DOREPLIFETIME(URpg_ContainerComponent, Containers);
	// DOREPLIFETIME(URpg_ContainerComponent, Slots);
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

void URpg_ContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
}

