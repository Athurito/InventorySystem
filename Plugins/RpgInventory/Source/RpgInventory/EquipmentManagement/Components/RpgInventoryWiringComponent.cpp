// Fill out your copyright notice in the Description page of Project Settings.

#include "RpgInventoryWiringComponent.h"

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

void URpgInventoryWiringComponent::BeginPlay()
{
	Super::BeginPlay();

	RegisterInitStateFeature();

	// No timers: re-check init when the Pawn's PlayerState is replicated/assigned.
	if (!OwnerPropertyChangedHandle.IsValid())
	{
		OwnerPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(
			this, &URpgInventoryWiringComponent::HandleOwnerPropertyChanged);
	}

	TryToChangeInitState(RpgTags::InitState_Spawned);
	CheckDefaultInitialization();
}

void URpgInventoryWiringComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerPropertyChangedHandle.IsValid())
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OwnerPropertyChangedHandle);
		OwnerPropertyChangedHandle.Reset();
	}

	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

void URpgInventoryWiringComponent::HandleOwnerPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	// We only care about our owning Pawn and its PlayerState being set/replicated.
	if (!Object || Object != GetOwner())
	{
		return;
	}

	const FProperty* ChangedProperty = PropertyChangedEvent.Property;
	if (!ChangedProperty)
	{
		return;
	}

	// Pawn has a property named "PlayerState" (replicatedUsing=OnRep_PlayerState). It's private, so use name.
	static const FName PlayerStatePropertyName(TEXT("PlayerState"));
	if (ChangedProperty->GetFName() == PlayerStatePropertyName)
	{
		CheckDefaultInitialization();
	}
}

FName URpgInventoryWiringComponent::GetFeatureName() const
{
	return RpgInventoryWiring::FeatureName;
}

void URpgInventoryWiringComponent::CheckDefaultInitialization()
{
	ContinueInitStateChain(RpgInventoryWiring::InitStateChain);
}

bool URpgInventoryWiringComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	if (!DesiredState.IsValid())
	{
		return false;
	}

	if (DesiredState == RpgTags::InitState_Spawned)
	{
		return true;
	}

	// We need a valid PlayerState + InventoryManager for anything beyond Spawned.
	if (DesiredState == RpgTags::InitState_DataAvailable)
	{
		return ResolveInventoryManager() != nullptr;
	}

	if (DesiredState == RpgTags::InitState_DataInitialized)
	{
		UInventoryManagerComponent* IM = ResolveInventoryManager();
		if (!IM)
		{
			return false;
		}

		// Container definitions must be present so the character systems can initialize immediately.
		return IM->GetFirstContainerIndexByType(EInventorySlotType::Hotbar) != INDEX_NONE;
	}

	if (DesiredState == RpgTags::InitState_GameplayReady)
	{
		return bCharacterSystemsInitialized;
	}

	return false;
}

void URpgInventoryWiringComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
	if (DesiredState == RpgTags::InitState_DataInitialized)
	{
		UInventoryManagerComponent* IM = ResolveInventoryManager();
		if (IM)
		{
			InitializeCharacterSystems(IM);
		}
	}
}

UInventoryManagerComponent* URpgInventoryWiringComponent::ResolveInventoryManager() const
{
	APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!OwningPawn)
	{
		return nullptr;
	}

	APlayerState* PS = OwningPawn->GetPlayerState();
	if (!PS)
	{
		return nullptr;
	}

	return PS->FindComponentByClass<UInventoryManagerComponent>();
}

void URpgInventoryWiringComponent::InitializeCharacterSystems(UInventoryManagerComponent* InventoryManager)
{
	if (bCharacterSystemsInitialized)
	{
		return;
	}
	if (!InventoryManager)
	{
		return;
	}
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	UActiveItemComponent* ActiveItem = OwnerActor->FindComponentByClass<UActiveItemComponent>();
	if (ActiveItem)
	{
		ActiveItem->InitializeFromInventory(InventoryManager);
	}

	if (UEquipmentManagerComponent* EquipMgr = OwnerActor->FindComponentByClass<UEquipmentManagerComponent>())
	{
		EquipMgr->InitializeFromInventory(InventoryManager, ActiveItem);
	}

	if (UWeaponManagerComponent* WeaponMgr = OwnerActor->FindComponentByClass<UWeaponManagerComponent>())
	{
		WeaponMgr->InitializeFromInventory(InventoryManager, ActiveItem);
	}

	bCharacterSystemsInitialized = true;

	// Once we initialize, try to progress to GameplayReady.
	TryToChangeInitState(RpgTags::InitState_GameplayReady);
}
