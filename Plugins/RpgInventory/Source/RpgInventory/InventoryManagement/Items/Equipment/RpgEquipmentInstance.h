// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "RpgEquipmentInstance.generated.h"

class URpgEquipmentDefinition;
class UInventoryItemInstance;

/**
 * Instance of an equipped item. Stores spawned actors and GAS handles.
 */
UCLASS(BlueprintType)
class RPGINVENTORY_API URpgEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UInventoryItemInstance> SourceItem;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	TArray<FGameplayAbilitySpecHandle> AbilityHandles;
	TArray<FActiveGameplayEffectHandle> EffectHandles;

	void DestroySpawnedActors();
};
