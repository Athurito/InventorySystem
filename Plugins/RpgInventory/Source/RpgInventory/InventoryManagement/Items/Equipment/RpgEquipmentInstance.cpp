// Fill out your copyright notice in the Description page of Project Settings.

#include "RpgEquipmentInstance.h"
#include "Net/UnrealNetwork.h"

void URpgEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, SourceItem);
	DOREPLIFETIME(ThisClass, SpawnedActors);
	DOREPLIFETIME(ThisClass, AbilityHandles);
	DOREPLIFETIME(ThisClass, EffectHandles);
}

void URpgEquipmentInstance::DestroySpawnedActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();
}
