// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Interact/Rpg_InteractableComponent.h"
#include "GameFramework/Pawn.h"

URpg_InteractableComponent::URpg_InteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FInteractDisplayData URpg_InteractableComponent::GetDisplayData_Implementation() const
{
	return InteractableData ? InteractableData->Display : FInteractDisplayData();
}

bool URpg_InteractableComponent::CanInteract_Implementation(APawn* Instigator) const
{
	if (!bEnabled || !GetOwner()) return false;
	if (!Instigator) return false;

	const float Dist = FVector::Dist(Instigator->GetActorLocation(), GetOwner()->GetActorLocation());
	return Dist <= MaxUseDistance;
}

void URpg_InteractableComponent::Interact_Implementation(APawn* Instigator)
{
	OnInteraction.Broadcast(Instigator);
}
