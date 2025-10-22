// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Rpg_ContainerComponent.h"


#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "Items/Components/Rpg_ItemComponent.h"
#include "Items/Rpg_ItemDefinition.h"
#include "Items/Fragments/ConsumableFragment.h"
#include "Items/Fragments/StackableFragment.h"
#include "Net/UnrealNetwork.h"
#include "Engine/AssetManager.h"
#include "InventoryManagement/Utils/InventoryStatics.h"

URpg_ContainerComponent::URpg_ContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URpg_ContainerComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URpg_ContainerComponent, InventoryList);
}


void URpg_ContainerComponent::TryConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity)
{
	if (!ItemComponent || Quantity <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryConsumeItem: Invalid args (Item=%p, Qty=%d)"), ItemComponent, Quantity);
		return;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InternalConsume(ItemComponent, Quantity);
	}
	else
	{
		ServerConsumeItem(ItemComponent, Quantity);
	}
}

void URpg_ContainerComponent::AddRepSubObject(UObject* SubObject)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObject))
	{
		AddReplicatedSubObject(SubObject);
	}
}

void URpg_ContainerComponent::ServerConsumeItem_Implementation(URpg_ItemComponent* ItemComponent, const int32 Quantity)
{
	if (!ItemComponent || !IsValid(ItemComponent) || !IsValid(ItemComponent->GetOwner())) return;
	InternalConsume(ItemComponent, Quantity);
}

bool URpg_ContainerComponent::InternalConsume(URpg_ItemComponent* ItemComponent, int32 const Quantity) const
{
	ensure(GetOwner() && GetOwner()->HasAuthority());
	
	if (!ensure(ItemComponent))
	{
		return false;
	}

	const URpg_ItemDefinition* ItemDefinition = ItemComponent->GetItemDefinition();
	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("InternalConsume: ItemData is null"));
		return false;
	}
	
	const FConsumableFragment* Consumable = ItemDefinition->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment);
	if (!Consumable)
	{
		UE_LOG(LogTemp, Warning, TEXT("InternalConsume: Item not consumable"));
		return false;
	}

	const int32 PerUse = FMath::Max(1, Consumable->QuantityPerUse);
	const int32 Avail = ItemComponent->GetCurrentStackCount();
	const int32 MaxUses = Consumable->bReduceStack ? (Avail / PerUse) : Quantity;
	const int32 Uses = FMath::Clamp(Quantity, 0, MaxUses);

	if (Uses <= 0) return false;
	
	for (int32 i = 0; i < Uses; ++i)
	{
		ItemComponent->Consume(ResolveInstigator(ItemComponent)); // verringert serverseitig Stack
	}

	OnItemConsumed.Broadcast(ItemComponent, Uses);

	// Dropped-Item: bei 0 Stack → Owner zerstören (in ItemComponent::Consume oder hier)
	if (ItemComponent->GetCurrentStackCount() <= 0)
	{
		if (AActor* Owner = ItemComponent->GetOwner()) Owner->Destroy();
	}
	return true;
}

void URpg_ContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	Containers.Reset();
	for (const auto& SoftDef : InitialContainerDefs)
	{
		const UInventoryContainerDefinition* Def = SoftDef.IsValid() ? SoftDef.Get() : SoftDef.LoadSynchronous();
		if (!Def) continue;

		FInvContainer C(this);
		C.DisplayName  = Def->DisplayName;
		C.Type         = Def->Type;
		C.Rows         = Def->Rows;
		C.Cols         = Def->Cols;
		C.AllowedItems = Def->AllowedItems;
		C.TabIcon      = Def->TabIcon;
		Containers.Add(MoveTemp(C));
	}
}

APawn* URpg_ContainerComponent::ResolveInstigator(const URpg_ItemComponent* ItemComponent) const
{
	APawn* InstigatorPawn = nullptr;
	AActor* OwnerActor = GetOwner();
	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		InstigatorPawn = OwnerPawn;
	}
	else if (APlayerController* PC = Cast<APlayerController>(OwnerActor))
	{
		InstigatorPawn = PC->GetPawn();
	}
	else if (APlayerState* PS = Cast<APlayerState>(OwnerActor))
	{
		if (AController* C = PS->GetOwningController())
		{
			InstigatorPawn = C->GetPawn();
		}
	}
	
	if (!InstigatorPawn)
	{
		// Fallback: try the item's owner as instigator pawn
		InstigatorPawn = Cast<APawn>(ItemComponent->GetOwner());
	}
	return InstigatorPawn;
}


bool URpg_ContainerComponent::InternalAddItem(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	OutAdded = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!ItemComponent || Quantity <= 0) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;

	const URpg_ItemDefinition* Def = ItemComponent->GetItemDefinition();
	if (!Def) return false;

	return InternalAddItemById(ContainerIndex, Def->GetPrimaryAssetId(), Quantity, OutAdded, OutInstanceId);
}

bool URpg_ContainerComponent::InternalRemoveItem(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved)
{
	OutRemoved = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;
	return Containers[ContainerIndex].RemoveByInstance(InstanceId, Quantity, OutRemoved);
}

bool URpg_ContainerComponent::InternalAddItemById(int32 ContainerIndex, const FPrimaryAssetId& ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	OutAdded = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (Quantity <= 0) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;
	
	URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(ItemId);

	if (!Def)
	{
		return false; // could not resolve definition from id
	}
	
 	FInvContainer& Cont = Containers[ContainerIndex];
	const FGameplayTag ItemType = Def->GetItemType();
	const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
	const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

	int32 Added = 0;
	FGuid UsedInstance;
	const int32 LastIndex = Cont.AddOrStack(ItemId, ItemType, MaxStack, Quantity, UsedInstance, Added);
	OutAdded = Added;
	OutInstanceId = UsedInstance;
	return LastIndex != INDEX_NONE || Added > 0;
}

bool URpg_ContainerComponent::AddItemToContainerById(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalAddItemById(ContainerIndex, ItemId, Quantity, OutAdded, OutInstanceId);
	}
	else
	{
		ServerAddItemToContainerById(ContainerIndex, ItemId, Quantity);
		OutAdded = 0; // will update via replication
		OutInstanceId.Invalidate();
		return false;
	}
}

bool URpg_ContainerComponent::InternalTransferItem(URpg_ContainerComponent* TargetComponent, int32 SourceContainerIndex, int32 TargetContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutMoved)
{
	OutMoved = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!TargetComponent) return false;
	if (!Containers.IsValidIndex(SourceContainerIndex)) return false;
	if (!TargetComponent->Containers.IsValidIndex(TargetContainerIndex)) return false;

	FInvContainer& Src = Containers[SourceContainerIndex];
	FInvContainer& Dst = TargetComponent->Containers[TargetContainerIndex];

	// Block transfer within the same component if types are identical (player inventory rule)
	if (TargetComponent == this && Src.Type == Dst.Type)
	{
		return false;
	}
	const int32 SrcIdx = Src.FindIndexByInstance(InstanceId);
	if (SrcIdx == INDEX_NONE) return false;
	const FInv_InventoryEntry SrcEntry = Src.GetEntries()[SrcIdx];
	// Respect destination allowed items
	if (!Dst.IsItemAllowed(SrcEntry.GetItemType())) return false;

	int32 Remaining = Quantity;
	if (Remaining <= 0) return false;

	URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(SrcEntry.GetItemId());

	if (!Def)
	{
		return false; // could not resolve definition from id
	}
	
	const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
	const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

	FGuid NewInstanceId = InstanceId;
	int32 Added = 0;
	Dst.AddOrStack(SrcEntry.GetItemId(), SrcEntry.GetItemType(), MaxStack, Remaining, NewInstanceId, Added);
	if (Added <= 0) return false;
	int32 Removed = 0;
	Src.RemoveByInstance(InstanceId, Added, Removed);
	OutMoved = FMath::Min(Added, Removed);
	return OutMoved > 0;
}

bool URpg_ContainerComponent::AddItemToContainer(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalAddItem(ContainerIndex, ItemComponent, Quantity, OutAdded, OutInstanceId);
	}
	else
	{
		ServerAddItemToContainer(ContainerIndex, ItemComponent, Quantity);
		OutAdded = 0; // will update via replication
		OutInstanceId.Invalidate();
		return false;
	}
}

bool URpg_ContainerComponent::RemoveItemFromContainer(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalRemoveItem(ContainerIndex, InstanceId, Quantity, OutRemoved);
	}
	else
	{
		ServerRemoveItemFromContainer(ContainerIndex, InstanceId, Quantity);
		OutRemoved = 0;
		return false;
	}
}

bool URpg_ContainerComponent::TransferItem(URpg_ContainerComponent* TargetComponent, int32 SourceContainerIndex, int32 TargetContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutMoved)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalTransferItem(TargetComponent, SourceContainerIndex, TargetContainerIndex, InstanceId, Quantity, OutMoved);
	}
	else
	{
		ServerTransferItem(TargetComponent, SourceContainerIndex, TargetContainerIndex, InstanceId, Quantity);
		OutMoved = 0;
		return false;
	}
}

void URpg_ContainerComponent::ServerAddItemToContainer_Implementation(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	int32 DummyAdded; FGuid DummyId; InternalAddItem(ContainerIndex, ItemComponent, Quantity, DummyAdded, DummyId);
}

void URpg_ContainerComponent::ServerRemoveItemFromContainer_Implementation(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	int32 DummyRemoved; InternalRemoveItem(ContainerIndex, InstanceId, Quantity, DummyRemoved);
}

void URpg_ContainerComponent::ServerAddItemToContainerById_Implementation(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity)
{
	int32 DummyAdded; FGuid DummyId; InternalAddItemById(ContainerIndex, ItemId, Quantity, DummyAdded, DummyId);
}

void URpg_ContainerComponent::ServerTransferItem_Implementation(URpg_ContainerComponent* TargetComponent, int32 SourceContainerIndex, int32 TargetContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	int32 Dummy; InternalTransferItem(TargetComponent, SourceContainerIndex, TargetContainerIndex, InstanceId, Quantity, Dummy);
}
