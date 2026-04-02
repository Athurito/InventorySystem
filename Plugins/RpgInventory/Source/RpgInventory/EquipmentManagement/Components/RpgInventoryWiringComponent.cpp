// Fill out your copyright notice in the Description page of Project Settings.

#include "RpgInventoryWiringComponent.h"

#include "IMotionController.h"
#include "UObject/UObjectGlobals.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include "Components/GameFrameworkComponentManager.h"

#include "RpgInventory/EquipmentManagement/Components/ActiveItemComponent.h"
#include "RpgInventory/EquipmentManagement/Components/EquipmentManagerComponent.h"
#include "RpgInventory/EquipmentManagement/Components/WeaponManagerComponent.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/GameplayTags/InventoryInitStateTags.h"

namespace RpgInventoryWiring
{
	static const FName FeatureName(TEXT("RpgInventoryWiring"));

	static const TArray<FGameplayTag> InitStateChain = {
		RpgTags::InitState_Spawned,
		RpgTags::InitState_DataAvailable,
		RpgTags::InitState_DataInitialized,
		RpgTags::InitState_GameplayReady
	};
}

URpgInventoryWiringComponent::URpgInventoryWiringComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool URpgInventoryWiringComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);
	APawn* Pawn = GetPawn<APawn>();
	
	if (!Pawn) return false;
	
	//---------- Spawned ------------
	if (!CurrentState.IsValid() && DesiredState == RpgTags::InitState_Spawned)
	{
		return true;
	}
	
	//---------- DataAvailable ------------
	if (CurrentState == RpgTags::InitState_Spawned && DesiredState == RpgTags::InitState_DataAvailable)
	{
		return true;
	}
	//---------- DataInitialized ------------
	if (CurrentState == RpgTags::InitState_DataAvailable && DesiredState == RpgTags::InitState_DataInitialized)
	{
		Manager->HasFeatureReachedInitState(Pawn, RpgInventoryWiring::FeatureName, RpgTags::InitState_DataInitialized);
	}
	//---------- GameplayReady ------------
	if (CurrentState == RpgTags::InitState_DataInitialized && DesiredState == RpgTags::InitState_GameplayReady)
	{
		return true;
	}
	
	
}

void URpgInventoryWiringComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	IGameFrameworkInitStateInterface::HandleChangeInitState(Manager, CurrentState, DesiredState);
}

void URpgInventoryWiringComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	IGameFrameworkInitStateInterface::OnActorInitStateChanged(Params);
}

void URpgInventoryWiringComponent::CheckDefaultInitialization()
{
	ContinueInitStateChain(RpgInventoryWiring::InitStateChain);
}

void URpgInventoryWiringComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URpgInventoryWiringComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	Super::EndPlay(EndPlayReason);
}

void URpgInventoryWiringComponent::OnRegister()
{
	Super::OnRegister();
	if (GetPawn<APawn>())
	{
		RegisterInitStateFeature();
	}
}


