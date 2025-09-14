// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Interact/Rpg_InteractableComponent.h"

URpg_InteractableComponent::URpg_InteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

FInteractDisplayData URpg_InteractableComponent::GetDisplayData() const
{
	return DataAsset ? DataAsset->Display : InlineDisplayData;
}

bool URpg_InteractableComponent::CanInteract(AActor* Instigator) const
{
	if (!bEnabled || !GetOwner()) return false;
	if (!Instigator) return false;

	const float Dist = FVector::Dist(Instigator->GetActorLocation(), GetOwner()->GetActorLocation());
	return Dist <= MaxUseDistance;
}
