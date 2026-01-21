// Fill out your copyright notice in the Description page of Project Settings.

#include "RpgEquipmentInstance.h"

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
