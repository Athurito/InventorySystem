// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentManagerComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Equippable.h"

UEquipmentManagerComponent::UEquipmentManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


void UEquipmentManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Falls die Komponente am Character hängt, suchen wir den InventoryManager am PlayerState
	if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerState* PS = OwningPawn->GetPlayerState())
		{
			InventoryManager = PS->FindComponentByClass<UInventoryManagerComponent>();
		}
	}
	
	// Fallback: Falls die Komponente direkt am PlayerState (oder dem gleichen Actor wie der Manager) hängt
	if (!InventoryManager)
	{
		InventoryManager = GetOwner()->FindComponentByClass<UInventoryManagerComponent>();
	}

	if (InventoryManager)
	{
		EquipmentContainerIndex = InventoryManager->GetFirstContainerIndexByType(EInventorySlotType::Equipment);
		if (EquipmentContainerIndex != INDEX_NONE)
		{
			InventoryManager->OnInventorySlotChanged.AddDynamic(this, &UEquipmentManagerComponent::OnInventorySlotChanged);
			
			// Initialen Stand laden
			int32 NumSlots = InventoryManager->GetNumSlots(EquipmentContainerIndex);
			CurrentEquipment.SetNum(NumSlots);
			for (int32 i = 0; i < NumSlots; ++i)
			{
				RefreshSlot(i);
			}
		}
	}
}

void UEquipmentManagerComponent::OnInventorySlotChanged(int32 ContainerIndex, int32 SlotIndex)
{
	if (ContainerIndex == EquipmentContainerIndex)
	{
		RefreshSlot(SlotIndex);
	}
}

void UEquipmentManagerComponent::RefreshSlot(int32 SlotIndex)
{
	if (!InventoryManager || EquipmentContainerIndex == INDEX_NONE) return;

	UInventoryItemInstance* NewItem = InventoryManager->GetItemInstanceInSlot(SlotIndex, EquipmentContainerIndex);
	UInventoryItemInstance* OldItem = CurrentEquipment.IsValidIndex(SlotIndex) ? CurrentEquipment[SlotIndex] : nullptr;

	if (NewItem != OldItem)
	{
		if (OldItem)
		{
			OnItemUnequipped(SlotIndex, OldItem);
		}
		
		if (CurrentEquipment.IsValidIndex(SlotIndex))
		{
			CurrentEquipment[SlotIndex] = NewItem;
		}
		else
		{
			CurrentEquipment.SetNum(SlotIndex + 1);
			CurrentEquipment[SlotIndex] = NewItem;
		}

		if (NewItem)
		{
			OnItemEquipped(SlotIndex, NewItem);
		}
	}
}

void UEquipmentManagerComponent::OnItemEquipped(int32 SlotIndex, UInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance) return;

	const UInventoryFragment_Equippable* EquipFrag = ItemInstance->FindFragmentByClass<UInventoryFragment_Equippable>();
	if (!EquipFrag) return;

	FActiveEquipmentSlot& ActiveSlot = ActiveSlots.FindOrAdd(SlotIndex);

	// 1) Gameplay Effects anwenden
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			for (auto& EffectClass : EquipFrag->GameplayEffects)
			{
				if (EffectClass)
				{
					FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
					Context.AddInstigator(GetOwner(), GetOwner());
					FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
					if (Spec.IsValid())
					{
						FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
						ActiveSlot.AppliedEffects.Add(Handle);
					}
				}
			}
		}
	}

	// 2) Mesh attachen
	if (!EquipFrag->SkeletalMesh.IsNull())
	{
		// Wir suchen das Haupt-Mesh des Charakters
		USkeletalMeshComponent* MainMesh = Cast<USkeletalMeshComponent>(GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		if (MainMesh)
		{
			USkeletalMeshComponent* NewPart = NewObject<USkeletalMeshComponent>(GetOwner());
			NewPart->SetSkeletalMesh(EquipFrag->SkeletalMesh.LoadSynchronous());
			NewPart->RegisterComponent();
			NewPart->AttachToComponent(MainMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, EquipFrag->SocketName);
			ActiveSlot.SpawnedComponent = NewPart;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Equipped item %s in slot %d"), *ItemInstance->GetName(), SlotIndex);
}

void UEquipmentManagerComponent::OnItemUnequipped(int32 SlotIndex, UInventoryItemInstance* ItemInstance)
{
	FActiveEquipmentSlot* ActiveSlot = ActiveSlots.Find(SlotIndex);
	if (!ActiveSlot) return;

	// 1) Effekte entfernen
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			for (auto& Handle : ActiveSlot->AppliedEffects)
			{
				if (Handle.IsValid())
				{
					ASC->RemoveActiveGameplayEffect(Handle);
				}
			}
		}
	}

	// 2) Komponente zerstören
	if (ActiveSlot->SpawnedComponent)
	{
		ActiveSlot->SpawnedComponent->DestroyComponent();
	}

	ActiveSlots.Remove(SlotIndex);
	UE_LOG(LogTemp, Log, TEXT("Unequipped item from slot %d"), SlotIndex);
}
