// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "GameplayTagContainer.h"
#include "InventoryFragment_Equippable.generated.h"

class UGameplayEffect;
class USkeletalMesh;

/**
 * Fragment für ausrüstbare Items (Waffen, Rüstung etc.)
 */
UCLASS()
class RPGINVENTORY_API UInventoryFragment_Equippable : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	// Tags, die dieses Item beschreiben (z.B. Ausrüstung.Kopf, Waffe.Schwert)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGameplayTagContainer EquipmentTags;

	// Effekte, die beim Ausrüsten angewendet werden (Stats, Buffs)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<TSubclassOf<UGameplayEffect>> GameplayEffects;

	// Visuelle Darstellung (Mesh das gespawnt oder getauscht wird)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	// Wo am Charakter soll das Mesh befestigt werden?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	FName SocketName;
};
