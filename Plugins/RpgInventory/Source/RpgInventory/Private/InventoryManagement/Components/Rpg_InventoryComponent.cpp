// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Rpg_InventoryComponent.h"


// Sets default values for this component's properties
URpg_InventoryComponent::URpg_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void URpg_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void URpg_InventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

