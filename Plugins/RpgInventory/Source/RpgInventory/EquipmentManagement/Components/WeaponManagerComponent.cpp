// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponManagerComponent.h"

#include "ActiveItemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/InventoryItemInstance.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_WeaponConfig.h"

UWeaponManagerComponent::UWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	// Wiring happens via `URpgInventoryWiringComponent`.
}

void UWeaponManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InventoryManager)
	{
		InventoryManager->OnInventorySlotChanged.RemoveDynamic(this, &UWeaponManagerComponent::OnInventorySlotChanged);
	}
	if (ActiveItemComponent)
	{
		ActiveItemComponent->OnActiveHotbarSlotChanged.RemoveDynamic(this, &UWeaponManagerComponent::OnActiveSlotChanged);
	}

	// Cleanup spawned actors on server
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		for (TPair<int32, FSpawnedWeaponSlot>& Pair : SpawnedWeapons)
		{
			if (Pair.Value.SpawnedActor)
			{
				Pair.Value.SpawnedActor->Destroy();
			}
		}
		SpawnedWeapons.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void UWeaponManagerComponent::InitializeFromInventory(UInventoryManagerComponent* InInventoryManager, UActiveItemComponent* InActiveItemComponent)
{
	if (bInitialized)
	{
		return;
	}

	InventoryManager = InInventoryManager;
	ActiveItemComponent = InActiveItemComponent;

	if (ActiveItemComponent)
	{
		ActiveItemComponent->OnActiveHotbarSlotChanged.AddUniqueDynamic(this, &UWeaponManagerComponent::OnActiveSlotChanged);
	}

	if (!InventoryManager)
	{
		return;
	}

	HotbarContainerIndex = InventoryManager->GetFirstContainerIndexByType(EInventorySlotType::Hotbar);
	if (HotbarContainerIndex == INDEX_NONE)
	{
		return;
	}

	InventoryManager->OnInventorySlotChanged.AddUniqueDynamic(this, &UWeaponManagerComponent::OnInventorySlotChanged);

	const int32 NumSlots = InventoryManager->GetNumSlots(HotbarContainerIndex);
	CurrentHotbar.SetNum(NumSlots);
	for (int32 i = 0; i < NumSlots; ++i)
	{
		RefreshHotbarSlot(i);
	}

	bInitialized = true;
	UE_LOG(LogTemp, Warning, TEXT("[WeaponMgrInit] Owner=%s PS=%s InvOwner=%s HotbarContainerIndex=%d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetPlayerState() : nullptr),
		*GetNameSafe(InventoryManager ? InventoryManager->GetOwner() : nullptr),
		HotbarContainerIndex);
}

void UWeaponManagerComponent::OnInventorySlotChanged(int32 ContainerIndex, int32 SlotIndex)
{
	if (ContainerIndex != HotbarContainerIndex)
	{
		return;
	}
	RefreshHotbarSlot(SlotIndex);
}

void UWeaponManagerComponent::OnActiveSlotChanged(int32 NewActiveSlotIndex, int32 OldActiveSlotIndex)
{
	// Update both involved slots (if any) to avoid iterating all
	if (OldActiveSlotIndex != INDEX_NONE)
	{
		UpdateAttachmentForSlot(OldActiveSlotIndex);
	}
	if (NewActiveSlotIndex != INDEX_NONE)
	{
		UpdateAttachmentForSlot(NewActiveSlotIndex);
	}
}

void UWeaponManagerComponent::RefreshHotbarSlot(int32 SlotIndex)
{
	if (!InventoryManager || HotbarContainerIndex == INDEX_NONE)
	{
		return;
	}

	UInventoryItemInstance* NewItem = InventoryManager->GetItemInstanceInSlot(SlotIndex, HotbarContainerIndex);
	UInventoryItemInstance* OldItem = CurrentHotbar.IsValidIndex(SlotIndex) ? CurrentHotbar[SlotIndex] : nullptr;

	if (NewItem == OldItem)
	{
		// Still may need to update attachment if active changed elsewhere
		UpdateAttachmentForSlot(SlotIndex);
		return;
	}

	// Unequipped
	if (OldItem)
	{
		DestroyWeaponActorForSlot(SlotIndex);
	}

	if (CurrentHotbar.IsValidIndex(SlotIndex))
	{
		CurrentHotbar[SlotIndex] = NewItem;
	}
	else
	{
		CurrentHotbar.SetNum(SlotIndex + 1);
		CurrentHotbar[SlotIndex] = NewItem;
	}

	// Equipped
	if (NewItem)
	{
		SpawnWeaponActorForItem(SlotIndex, NewItem);
		UpdateAttachmentForSlot(SlotIndex);
	}
}

AActor* UWeaponManagerComponent::SpawnWeaponActorForItem(int32 SlotIndex, UInventoryItemInstance* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return nullptr;
	}
	if (!Item)
	{
		return nullptr;
	}

	const UInventoryFragment_WeaponConfig* WeaponFrag = Item->FindFragmentByClass<UInventoryFragment_WeaponConfig>();
	if (!WeaponFrag || !WeaponFrag->WeaponActorClass)
	{
		return nullptr;
	}

	FSpawnedWeaponSlot& Slot = SpawnedWeapons.FindOrAdd(SlotIndex);
	if (Slot.SpawnedActor)
	{
		return Slot.SpawnedActor;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = Cast<APawn>(GetOwner());
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Spawned = World->SpawnActor<AActor>(WeaponFrag->WeaponActorClass, FTransform::Identity, Params);
	if (!Spawned)
	{
		return nullptr;
	}

	// Ensure replication for gameplay-relevant weapons. If the BP already sets this, it's harmless.
	Spawned->SetReplicates(true);

	Slot.SpawnedActor = Spawned;
	return Spawned;
}

void UWeaponManagerComponent::DestroyWeaponActorForSlot(int32 SlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (FSpawnedWeaponSlot* Slot = SpawnedWeapons.Find(SlotIndex))
	{
		if (Slot->SpawnedActor)
		{
			Slot->SpawnedActor->Destroy();
		}
		SpawnedWeapons.Remove(SlotIndex);
	}
}

void UWeaponManagerComponent::UpdateAttachmentForSlot(int32 SlotIndex)
{
	UInventoryItemInstance* Item = CurrentHotbar.IsValidIndex(SlotIndex) ? CurrentHotbar[SlotIndex] : nullptr;
	if (!Item)
	{
		return;
	}

	const UInventoryFragment_WeaponConfig* WeaponFrag = Item->FindFragmentByClass<UInventoryFragment_WeaponConfig>();
	if (!WeaponFrag)
	{
		return;
	}

	FSpawnedWeaponSlot* SpawnedSlot = SpawnedWeapons.Find(SlotIndex);
	if (!SpawnedSlot || !SpawnedSlot->SpawnedActor)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = GetCharacterMesh();
	if (!Mesh)
	{
		return;
	}

	const bool bSlotIsActive = ActiveItemComponent && (ActiveItemComponent->GetActiveSlotIndex() == SlotIndex);
	FName TargetSocket = bSlotIsActive ? WeaponFrag->EquipSocketName : WeaponFrag->StowedSocketName;
	if (TargetSocket.IsNone())
	{
		TargetSocket = WeaponFrag->EquipSocketName;
	}
	if (TargetSocket.IsNone())
	{
		return;
	}

	SpawnedSlot->SpawnedActor->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetIncludingScale, TargetSocket);
}

USkeletalMeshComponent* UWeaponManagerComponent::GetCharacterMesh() const
{
	return GetOwner() ? Cast<USkeletalMeshComponent>(GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass())) : nullptr;
}
