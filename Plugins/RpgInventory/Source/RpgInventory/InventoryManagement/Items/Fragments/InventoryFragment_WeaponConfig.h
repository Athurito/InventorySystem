// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemFragment.h"
#include "InventoryFragment_WeaponConfig.generated.h"

class UAnimMontage;
class UDataAsset;
class UInventoryAbilitySet;

/**
 * Data-only fragment for active weapons/tools that are represented by a spawned Actor.
 * The runtime handling (spawn/attach/montages/camera/anim switching) is done by a Character-side manager.
 */
UCLASS()
class RPGINVENTORY_API UInventoryFragment_WeaponConfig : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	/** Actor class to spawn for this weapon/tool (e.g. BP_WeaponBase child). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AActor> WeaponActorClass;

	/** Socket to attach to when the weapon/tool is active (in hand). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FName EquipSocketName;

	/** Socket to attach to when the weapon/tool is stowed/holstered. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FName StowedSocketName;

	/** Optional montages (purely data, played by the manager/abilities). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> EquipMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> UnequipMontage = nullptr;

	/** Optional data asset for animation set/layer config (project-specific). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TSubclassOf<UAnimInstance> AnimSet = nullptr;

	/** Optional camera settings/profile (project-specific). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Camera")
	TObjectPtr<UDataAsset> CameraSettings = nullptr;

	/**
	 * Ability sets to apply while this weapon/tool is the currently active item (in hand).
	 * Applied/removed by `UActiveItemComponent` (server-side via PlayerState ASC).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|AbilitySet")
	TArray<TObjectPtr<UInventoryAbilitySet>> ActiveAbilitySets;
};
