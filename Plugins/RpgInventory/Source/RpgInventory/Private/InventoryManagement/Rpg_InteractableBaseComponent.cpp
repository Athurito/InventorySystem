// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Rpg_InteractableBaseComponent.h"


// Sets default values for this component's properties
URpg_InteractableBaseComponent::URpg_InteractableBaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

