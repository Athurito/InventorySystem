// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentManagerComponent.h"


UEquipmentManagerComponent::UEquipmentManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


void UEquipmentManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}
