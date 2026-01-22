// Fill out your copyright notice in the Description page of Project Settings.

#include "RpgEquipmentManagerComponent.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Equippable.h"
#include "RpgInventory/InventoryManagement/Items/Equipment/RpgEquipmentDefinition.h"
#include "RpgInventory/InventoryManagement/Items/Equipment/RpgEquipmentInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/PlayerState.h"

URpgEquipmentManagerComponent::URpgEquipmentManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool URpgEquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (const FRpgEquipmentEntry& Entry : EquipmentEntries)
	{
		if (Entry.EquipmentInstance)
		{
			bWroteSomething |= Channel->ReplicateSubobject(Entry.EquipmentInstance, *Bunch, *RepFlags);
		}
	}

	return bWroteSomething;
}

void URpgEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquipmentEntries);
}

void URpgEquipmentManagerComponent::Initialize(UInventoryManagerComponent* InInventoryManager)
{
	UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] Initializing with InventoryManager: %s"), InInventoryManager ? *InInventoryManager->GetName() : TEXT("NULL"));
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

void URpgEquipmentManagerComponent::SetActiveHotbarSlot(int32 ContainerIndex, int32 SlotIndex)
{
	if (ActiveHotbarContainerIndex == ContainerIndex && ActiveHotbarSlotIndex == SlotIndex)
	{
		return;
	}

	// 1. Unequip current dynamic items (anything mapped from the old slot)
	FGameplayTag OldSlotTag = GetSlotTagForInventorySlot(ActiveHotbarContainerIndex, ActiveHotbarSlotIndex);
	if (OldSlotTag.IsValid())
	{
		UnequipItem(OldSlotTag);
	}

	ActiveHotbarContainerIndex = ContainerIndex;
	ActiveHotbarSlotIndex = SlotIndex;

	// 2. Equip new item if valid
	FGameplayTag NewSlotTag = GetSlotTagForInventorySlot(ActiveHotbarContainerIndex, ActiveHotbarSlotIndex);
	if (NewSlotTag.IsValid() && InventoryManager)
	{
		UInventoryItemInstance* ItemInstance = InventoryManager->GetItemInstanceInSlot(ActiveHotbarSlotIndex, ActiveHotbarContainerIndex);
		if (ItemInstance)
		{
			EquipItem(NewSlotTag, ItemInstance, true);
		}
	}
}

void URpgEquipmentManagerComponent::UseActiveItem()
{
	if (!InventoryManager) return;

	UInventoryItemInstance* ItemInstance = InventoryManager->GetItemInstanceInSlot(ActiveHotbarSlotIndex, ActiveHotbarContainerIndex);
	if (!ItemInstance) return;

	// Hier könnte man Consumable Fragment prüfen und Effekt anwenden
	// Oder eine allgemeine Use-Funktion im ItemInstance/Manager aufrufen
}

void URpgEquipmentManagerComponent::OnInventorySlotChanged(int32 ContainerIndex, int32 SlotIndex)
{
	FGameplayTag SlotTag = GetSlotTagForInventorySlot(ContainerIndex, SlotIndex);
	
	UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] OnInventorySlotChanged: Container=%d, Slot=%d, Tag=%s"), ContainerIndex, SlotIndex, *SlotTag.ToString());

	// If it's not a permanent slot tag, check if it's our active hotbar slot
	bool bIsActiveHotbarSlot = (ContainerIndex == ActiveHotbarContainerIndex && SlotIndex == ActiveHotbarSlotIndex);
	
	if (!SlotTag.IsValid() && !bIsActiveHotbarSlot) return;

	UInventoryItemInstance* ItemInstance = InventoryManager->GetItemInstanceInSlot(SlotIndex, ContainerIndex);
	
	UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] Item in slot: %s"), ItemInstance ? *ItemInstance->GetName() : TEXT("NULL"));

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
		EquipItem(SlotTag, ItemInstance, bIsActiveHotbarSlot);
	}
	else
	{
		if (FoundEntry)
		{
			UnequipItem(SlotTag);
		}
	}
}

void URpgEquipmentManagerComponent::EquipItem(FGameplayTag SlotTag, UInventoryItemInstance* ItemInstance, bool bIsDynamic)
{
	if (GetOwnerRole() != ROLE_Authority) return;

	UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] EquipItem: %s, Slot: %s, Dynamic: %d"), *ItemInstance->GetName(), *SlotTag.ToString(), bIsDynamic);

	const UInventoryFragment_Equippable* EquipFrag = ItemInstance->FindFragmentByClass<UInventoryFragment_Equippable>();
	if (!EquipFrag)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RpgEquipment] Item has no Equippable Fragment!"));
		return;
	}

	if (!EquipFrag->EquipmentDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RpgEquipment] Equippable Fragment has no EquipmentDefinition!"));
		return;
	}

	// Validation: If permanent slot, check if allowed. 
	// If dynamic (Hotbar), we usually just equip if it has a definition.
	if (!bIsDynamic)
	{
		if (!EquipFrag->SupportedSlots.HasTagExact(SlotTag))
		{
			UE_LOG(LogTemp, Warning, TEXT("[RpgEquipment] Slot %s not supported by item!"), *SlotTag.ToString());
			return;
		}
	}

	const URpgEquipmentDefinition* EquipDef = EquipFrag->EquipmentDefinition.Get();
	if (!EquipDef) return;

	URpgEquipmentInstance* NewInstance = NewObject<URpgEquipmentInstance>(this);
	NewInstance->SourceItem = ItemInstance;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] AbilitySystemComponent found: %s"), ASC ? *ASC->GetName() : TEXT("NULL"));

	// Grant Abilities
	if (ASC && GetOwnerRole() == ROLE_Authority)
	{
		for (auto AbilityClass : EquipDef->AbilitiesToGrant)
		{
			if (AbilityClass)
			{
				FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass));
				NewInstance->AbilityHandles.Add(Handle);
				UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] Granted Ability: %s"), *AbilityClass->GetName());
			}
		}

		for (auto EffectClass : EquipDef->EffectsToApply)
		{
			if (EffectClass)
			{
				FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(EffectClass.GetDefaultObject(), 1.0f, ASC->MakeEffectContext());
				NewInstance->EffectHandles.Add(Handle);
				UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] Applied Effect: %s"), *EffectClass->GetName());
			}
		}
	}

	// Spawn Actors
	for (const FRpgEquipmentActorToSpawn& ActorDef : EquipDef->ActorsToSpawn)
	{
		if (!ActorDef.ActorToSpawn) continue;

		AActor* NewActor = GetWorld()->SpawnActor<AActor>(ActorDef.ActorToSpawn, ActorDef.AttachTransform);
		if (NewActor)
		{
			UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] Spawned Actor: %s"), *NewActor->GetName());
			NewActor->SetReplicates(true);
			NewActor->SetReplicateMovement(true);

			// Attach to Character Mesh instead of RootComponent
			USceneComponent* AttachTarget = GetOwner()->GetRootComponent();
			
			APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
			// ->GetMes;
			// if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
			// {
			// 	AttachTarget = Character->GetMesh();
			// 	UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] Attaching to Character Mesh: %s"), *AttachTarget->GetName());
			// }
			ACharacter* Character = Cast<ACharacter>(PlayerState->GetPawn());
			NewActor->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, ActorDef.AttachSocket);
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
	if (GetOwnerRole() != ROLE_Authority) return;

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
	if (!ContainerDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RpgEquipment] No ContainerDefinition for index %d"), ContainerIndex);
		return FGameplayTag::EmptyTag;
	}

	if (ContainerDef->Type == EInventorySlotType::Equipment || ContainerDef->Type == EInventorySlotType::Hotbar)
	{
		if (ContainerDef->SlotTagMapping.Contains(SlotIndex))
		{
			FGameplayTag Tag = ContainerDef->SlotTagMapping[SlotIndex];
			UE_LOG(LogTemp, Log, TEXT("[RpgEquipment] Found SlotTag %s for C:%d S:%d"), *Tag.ToString(), ContainerIndex, SlotIndex);
			return Tag;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[RpgEquipment] Container %d has no mapping for Slot %d"), ContainerIndex, SlotIndex);
		}
	}

	return FGameplayTag::EmptyTag; 
}
