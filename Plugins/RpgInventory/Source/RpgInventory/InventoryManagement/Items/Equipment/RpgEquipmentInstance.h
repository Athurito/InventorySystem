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
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	TObjectPtr<UInventoryItemInstance> SourceItem;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SpawnedActors;

	UPROPERTY(Replicated)
	TArray<FGameplayAbilitySpecHandle> AbilityHandles;

	UPROPERTY(Replicated)
	TArray<FActiveGameplayEffectHandle> EffectHandles;

	void DestroySpawnedActors();
};
