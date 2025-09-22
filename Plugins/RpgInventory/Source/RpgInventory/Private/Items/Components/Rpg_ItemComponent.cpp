// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Components/Rpg_ItemComponent.h"

URpg_ItemComponent::URpg_ItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpg_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
}

void URpg_ItemComponent::InitItemData(UItemData CopyOfItemData)
{
}


FInteractDisplayData URpg_ItemComponent::GetDisplayData_Implementation() const
{
	return FInteractDisplayData();
}

bool URpg_ItemComponent::CanInteract_Implementation(APawn* Instigator) const
{
	return IInteractable::CanInteract_Implementation(Instigator);
}

void URpg_ItemComponent::Interact_Implementation(APawn* Instigator)
{
	IInteractable::Interact_Implementation(Instigator);
}

void URpg_ItemComponent::BeginPlay()
{
	Super::BeginPlay();
}

