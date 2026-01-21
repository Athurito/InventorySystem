// Fill out your copyright notice in the Description page of Project Settings.

#include "RpgEquipmentManagerComponent.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Equippable.h"
#include "RpgInventory/InventoryManagement/Items/Equipment/RpgEquipmentDefinition.h"
#include "RpgInventory/InventoryManagement/Items/Equipment/RpgEquipmentInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

URpgEquipmentManagerComponent::URpgEquipmentManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpgEquipmentManagerComponent::Initialize(UInventoryManagerComponent* InInventoryManager)
{
	InventoryManager = InInventoryManager;
	if (InventoryManager)
	{
		InventoryManager->OnInventorySlotChanged.AddDynamic(this, &URpgEquipmentManagerComponent::OnInventorySlotChanged);
	}
}

void URpgEquipmentManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URpgEquipmentManagerComponent::OnInventorySlotChanged(int32 ContainerIndex, int32 SlotIndex)
{
	FGameplayTag SlotTag = GetSlotTagForInventorySlot(ContainerIndex, SlotIndex);
	if (!SlotTag.IsValid()) return;

	UInventoryItemInstance* ItemInstance = InventoryManager->GetItemInstanceInSlot(SlotIndex, ContainerIndex);
	
	// Check what's currently in our record
	FRpgEquipmentEntry* FoundEntry = nullptr;
	for (FRpgEquipmentEntry& Entry : EquipmentEntries)
	{
		if (Entry.SlotTag == SlotTag)
		{
			FoundEntry = &Entry;
			break;
		}
	}

	if (ItemInstance)
	{
		if (FoundEntry)
		{
			if (FoundEntry->ItemInstance == ItemInstance) return; // No change
			UnequipItem(SlotTag);
		}
		EquipItem(SlotTag, ItemInstance);
	}
	else
	{
		if (FoundEntry)
		{
			UnequipItem(SlotTag);
		}
	}
}

void URpgEquipmentManagerComponent::EquipItem(FGameplayTag SlotTag, UInventoryItemInstance* ItemInstance)
{
	const UInventoryFragment_Equippable* EquipFrag = ItemInstance->FindFragmentByClass<UInventoryFragment_Equippable>();
	if (!EquipFrag || !EquipFrag->EquipmentDefinition) return;

	URpgEquipmentInstance* NewInstance = NewObject<URpgEquipmentInstance>(this);
	NewInstance->SourceItem = ItemInstance;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const URpgEquipmentDefinition* EquipDef = EquipFrag->EquipmentDefinition.GetDefaultObject();

	// Grant Abilities
	if (ASC && GetOwnerRole() == ROLE_Authority)
	{
		for (auto AbilityClass : EquipDef->AbilitiesToGrant)
		{
			NewInstance->AbilityHandles.Add(ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass)));
		}

		for (auto EffectClass : EquipDef->EffectsToApply)
		{
			NewInstance->EffectHandles.Add(ASC->ApplyGameplayEffectToSelf(EffectClass.GetDefaultObject(), 1.0f, ASC->MakeEffectContext()));
		}
	}

	// Spawn Actors
	for (const FRpgEquipmentActorToSpawn& ActorDef : EquipDef->ActorsToSpawn)
	{
		AActor* NewActor = GetWorld()->SpawnActor<AActor>(ActorDef.ActorToSpawn, ActorDef.AttachTransform);
		if (NewActor)
		{
			NewActor->AttachToComponent(Cast<USceneComponent>(GetOwner()->GetRootComponent()), FAttachmentTransformRules::KeepRelativeTransform, ActorDef.AttachSocket);
			NewInstance->SpawnedActors.Add(NewActor);
		}
	}

	FRpgEquipmentEntry NewEntry;
	NewEntry.SlotTag = SlotTag;
	NewEntry.ItemInstance = ItemInstance;
	NewEntry.EquipmentInstance = NewInstance;
	EquipmentEntries.Add(NewEntry);
}

void URpgEquipmentManagerComponent::UnequipItem(FGameplayTag SlotTag)
{
	int32 FoundIndex = INDEX_NONE;
	for (int32 i = 0; i < EquipmentEntries.Num(); ++i)
	{
		if (EquipmentEntries[i].SlotTag == SlotTag)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex != INDEX_NONE)
	{
		URpgEquipmentInstance* Instance = EquipmentEntries[FoundIndex].EquipmentInstance;
		if (Instance)
		{
			Instance->DestroySpawnedActors();

			UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
			if (ASC && GetOwnerRole() == ROLE_Authority)
			{
				for (auto Handle : Instance->AbilityHandles)
				{
					ASC->ClearAbility(Handle);
				}
				for (auto Handle : Instance->EffectHandles)
				{
					ASC->RemoveActiveGameplayEffect(Handle);
				}
			}
		}
		EquipmentEntries.RemoveAt(FoundIndex);
	}
}

UAbilitySystemComponent* URpgEquipmentManagerComponent::GetAbilitySystemComponent() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
}

FGameplayTag URpgEquipmentManagerComponent::GetSlotTagForInventorySlot(int32 ContainerIndex, int32 SlotIndex) const
{
	if (!InventoryManager) return FGameplayTag::EmptyTag;

	const UInventoryContainerDefinition* ContainerDef = InventoryManager->GetContainerDefinition(ContainerIndex);
	if (!ContainerDef || ContainerDef->Type != EInventorySlotType::Equipment)
	{
		return FGameplayTag::EmptyTag;
	}

	if (ContainerDef->SlotTagMapping.Contains(SlotIndex))
	{
		return ContainerDef->SlotTagMapping[SlotIndex];
	}

	return FGameplayTag::EmptyTag; 
}
