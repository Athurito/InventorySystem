// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RpgEquipmentDefinition.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class ARpgEquipmentActor;

USTRUCT(BlueprintType)
struct FRpgEquipmentActorToSpawn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Equipment)
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category=Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category=Equipment)
	FTransform AttachTransform;
};

/**
 * Definition of an equipment item, similar to Lyra.
 */
UCLASS(BlueprintType, Const)
class RPGINVENTORY_API URpgEquipmentDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	// Abilities to grant when equipped
	UPROPERTY(EditAnywhere, Category=Abilities)
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;

	// Effects to apply when equipped (e.g. Stats)
	UPROPERTY(EditAnywhere, Category=Abilities)
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	// Actors to spawn when equipped
	UPROPERTY(EditAnywhere, Category=Actors)
	TArray<FRpgEquipmentActorToSpawn> ActorsToSpawn;
};
