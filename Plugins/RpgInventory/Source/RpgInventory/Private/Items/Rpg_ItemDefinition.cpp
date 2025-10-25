// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Rpg_ItemDefinition.h"

UTexture2D* URpg_ItemDefinition::GetIcon() const
{
	return Icon.LoadSynchronous();
}

void URpg_ItemDefinition::SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation,
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

	// // Falls dein Pickup eine Schnittstelle oder Basisklasse hat, hier die Definition übergeben:
	// if (IPickupWithDefinition* AsPickup = Cast<IPickupWithDefinition>(Actor))
	// {
	// 	AsPickup->SetItemDefinition(this);
	// }
}
