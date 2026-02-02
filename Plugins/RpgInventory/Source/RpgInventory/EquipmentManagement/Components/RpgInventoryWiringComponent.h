// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/GameFrameworkInitStateInterface.h"

#include "RpgInventoryWiringComponent.generated.h"

class UInventoryManagerComponent;
class UEquipmentManagerComponent;
class UWeaponManagerComponent;
class UActiveItemComponent;

struct FPropertyChangedEvent;

/**
 * Lyra-style init/wiring component.
 *
 * Purpose: Avoid per-component timers by waiting for `PlayerState` + `UInventoryManagerComponent`
 * using the `ModularGameplay` init-state system, then initializing character-side systems.
 *
 * Add this component to your Character/Pawn (plugin-friendly: no base class required).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API URpgInventoryWiringComponent : public UActorComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	URpgInventoryWiringComponent();

	//~IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const override;
	virtual void CheckDefaultInitialization() override;
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UInventoryManagerComponent* ResolveInventoryManager() const;
	void HandleOwnerPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	void InitializeCharacterSystems(UInventoryManagerComponent* InventoryManager);

	bool bCharacterSystemsInitialized = false;
	FDelegateHandle OwnerPropertyChangedHandle;
};
