// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryFragment_Pickup.h"


void UInventoryFragment_Pickup::SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation,
	const FRotator& SpawnRotation)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPickupActor: WorldContextObject is null"));
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(WorldContextObject) : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPickupActor: World is null"));
		return;
	}

	if (!PickupActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPickupActor: PickupActorClass not set on %s"), *GetName());
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	auto ActorClass = PickupActorClass.LoadSynchronous();
	AActor* Actor = World->SpawnActor<AActor>(ActorClass, SpawnLocation, SpawnRotation, Params);
	if (!Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPickupActor: failed to spawn %s"), *PickupActorClass->GetName());
		return;
	}
}
