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
	DOREPLIFETIME(URpg_ContainerComponent, Containers);
}


void URpg_ContainerComponent::TryConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity)
{
	// Backward compatibility: route to unified world-use path
	TryUseWorldItem(ItemComponent, Quantity);
}

void URpg_ContainerComponent::TryUseItemByInstance(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	if (Quantity <= 0) return;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InternalUseItem_Inventory(ContainerIndex, InstanceId, Quantity);
	}
	else
	{
		ServerUseItemByInstance(ContainerIndex, InstanceId, Quantity);
	}
}

void URpg_ContainerComponent::TryUseWorldItem(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	if (!ItemComponent || Quantity <= 0) return;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InternalUseItem_World(ItemComponent, Quantity);
	}
	else
	{
		ServerUseWorldItem(ItemComponent, Quantity);
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
	// Route to unified world-use path
	ServerUseWorldItem(ItemComponent, Quantity);
}

bool URpg_ContainerComponent::InternalConsume(URpg_ItemComponent* ItemComponent, int32 const Quantity) const
{
	// Legacy path kept for compatibility; route into unified world use
	URpg_ContainerComponent* MutableThis = const_cast<URpg_ContainerComponent*>(this);
	return MutableThis->InternalUseItem_World(ItemComponent, Quantity);
}

void URpg_ContainerComponent::ServerUseItemByInstance_Implementation(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	InternalUseItem_Inventory(ContainerIndex, InstanceId, Quantity);
}

void URpg_ContainerComponent::ServerUseWorldItem_Implementation(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	InternalUseItem_World(ItemComponent, Quantity);
}

static const FConsumableFragment* GetConsumable(const URpg_ItemDefinition* Def)
{
	return Def ? Def->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment) : nullptr;
}

bool URpg_ContainerComponent::InternalUseItem_Inventory(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	if (!(GetOwner() && GetOwner()->HasAuthority())) return false;
	if (Quantity <= 0) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;

	FInv_InventoryEntry* Entry = Containers[ContainerIndex].FindEntryMutableByInstance(InstanceId);
	if (!Entry) return false;
	URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(Entry->GetItemId());
	if (!Def) return false;
	const FConsumableFragment* Cons = GetConsumable(Def);
	if (!Cons) return false;

	APawn* InstigatorPawn = ResolveInstigator(nullptr);
	if (!CanUseByFragment(Def, InstigatorPawn, EUseContext::Inventory)) return false;

	const int32 PerUse = FMath::Max(1, Cons->QuantityPerUse);

	// Cooldown
	if (Cons->CooldownSeconds > 0.f)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (!CheckAndSetCooldown(Entry->GetRuntimeDataMutable(), Def, Cons->CooldownSeconds, Now))
		{
			return false;
		}
	}

	const int32 AvailStack = Entry->GetStack();
	const int32 MaxUses = Cons->bReduceStack ? (AvailStack / PerUse) : Quantity;
	const int32 Uses = FMath::Clamp(Quantity, 0, MaxUses);
	if (Uses <= 0) return false;

	for (int32 i = 0; i < Uses; ++i)
	{
		ApplyUseByFragment(Def, InstigatorPawn);
	}

	ApplyCostsAndReplicate(Entry->GetRuntimeDataMutable(), Def, PerUse, Uses);

	// Remove entry if depleted
	if (Entry->GetStack() <= 0)
	{
		int32 Removed = 0;
		Containers[ContainerIndex].RemoveByInstance(InstanceId, 0, Removed);
	}

	OnItemConsumed.Broadcast(nullptr, Uses);
	return true;
}

bool URpg_ContainerComponent::InternalUseItem_World(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	if (!(GetOwner() && GetOwner()->HasAuthority())) return false;
	if (!ItemComponent || Quantity <= 0) return false;
	const URpg_ItemDefinition* Def = ItemComponent->GetItemDefinition();
	if (!Def) return false;
	const FConsumableFragment* Cons = GetConsumable(Def);
	if (!Cons) return false;

	APawn* InstigatorPawn = ResolveInstigator(ItemComponent);
	if (!CanUseByFragment(Def, InstigatorPawn, EUseContext::World)) return false;

	const int32 PerUse = FMath::Max(1, Cons->QuantityPerUse);

	// Cooldown
	if (Cons->CooldownSeconds > 0.f)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (!CheckAndSetCooldown(const_cast<FItemRuntimeDataContainer&>(ItemComponent->GetRuntimeData()), Def, Cons->CooldownSeconds, Now))
		{
			return false;
		}
	}

	const int32 AvailStack = ItemComponent->GetCurrentStackCount();
	const int32 MaxUses = Cons->bReduceStack ? (AvailStack / PerUse) : Quantity;
	const int32 Uses = FMath::Clamp(Quantity, 0, MaxUses);
	if (Uses <= 0) return false;

	for (int32 i = 0; i < Uses; ++i)
	{
		ItemComponent->Consume(InstigatorPawn);
	}

	if (ItemComponent->GetCurrentStackCount() <= 0)
	{
		if (AActor* Owner = ItemComponent->GetOwner()) Owner->Destroy();
	}

	OnItemConsumed.Broadcast(ItemComponent, Uses);
	return true;
}

bool URpg_ContainerComponent::CanUseByFragment(const URpg_ItemDefinition* Def, APawn* Instigator, EUseContext Ctx)
{
	const FConsumableFragment* Cons = GetConsumable(Def);
	if (!Cons) return false;

	switch (Cons->UseAvailability)
	{
		case EUseAvailability::WorldOnly: if (Ctx != EUseContext::World) return false; break;
		case EUseAvailability::InventoryOnly: if (Ctx == EUseContext::World) return false; break;
		case EUseAvailability::WorldOrInventory: break;
		case EUseAvailability::PickupThenUseIfWorld: if (Ctx == EUseContext::World) return false; break;
	}
	// Extend with BP checks later
	return true;
}

void URpg_ContainerComponent::ApplyUseByFragment(const URpg_ItemDefinition* Def, APawn* Instigator)
{
	// Currently effects are applied in URpg_ItemComponent::Consume for world items.
	// For inventory usage without a world component, you could mirror that logic here if needed.
	// No-op for now.
}

bool URpg_ContainerComponent::ApplyCostsAndReplicate(FItemRuntimeDataContainer& Runtime, const URpg_ItemDefinition* Def, int32 QuantityPerUse, int32 UsesToApply)
{
	bool bChanged = false;
	if (const FStackableFragment* Stackable = Def->GetFragmentOfTypeWithTag<FStackableFragment>(FragmentTags::StackableFragment))
	{
		if (auto* S = Runtime.FindOrAddMutable<FStackableRuntimeData>(FragmentTags::StackableFragment))
		{
			const int32 Max = FMath::Max(1, Stackable->GetMaxStackSize());
			const int32 Cost = QuantityPerUse * UsesToApply;
			S->CurrentStackCount = FMath::Clamp(S->CurrentStackCount - Cost, 0, Max);
			Runtime.MarkDirty(FragmentTags::StackableFragment);
			bChanged = true;
		}
	}
	return bChanged;
}

bool URpg_ContainerComponent::CheckAndSetCooldown(FItemRuntimeDataContainer& Runtime, const URpg_ItemDefinition* Def, float CooldownSeconds, float ServerTimeNow)
{
	if (CooldownSeconds <= 0.f) return true;
	if (auto* C = Runtime.FindOrAddMutable<FUseCooldownRuntimeData>(FragmentTags::ConsumableFragment))
	{
		if (ServerTimeNow - C->LastUseServerTime < CooldownSeconds)
		{
			return false;
		}
		C->LastUseServerTime = ServerTimeNow;
		Runtime.MarkDirty(FragmentTags::ConsumableFragment);
		return true;
	}
	return false;
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

	FInvContainer& Cont = Containers[ContainerIndex];
	const FGameplayTag ItemType = Def->GetItemType();
	const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
	const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

	int32 Added = 0;
	FGuid UsedInstance;
	const int32 LastIndex = Cont.AddOrStack(Def->GetPrimaryAssetId(), ItemType, MaxStack, Quantity, UsedInstance, Added);
	OutAdded = Added;
	OutInstanceId = UsedInstance;

	// If a new stack was created, copy runtime data from the world item component into the new entry
	if (LastIndex != INDEX_NONE)
	{
		FInv_InventoryEntry* NewEntry = Cont.FindEntryMutableByInstance(UsedInstance);
		if (NewEntry)
		{
			NewEntry->CopyRuntimeDataFrom(ItemComponent->GetRuntimeData());
			// Ensure the stack count matches what the container decided for the new stack
			NewEntry->SetStack(NewEntry->GetStack());
		}
	}
	return LastIndex != INDEX_NONE || Added > 0;
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

bool URpg_ContainerComponent::AutoDepositMatchingTo(URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex, int32& OutTotalMoved)
{
	OutTotalMoved = 0;

	// Route to server if not authoritative
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerAutoDepositMatchingTo(TargetComponent, TargetContainerIndex);
		return false; // UI will update via replication callbacks
	}

	if (!TargetComponent) return false;
	if (!TargetComponent->Containers.IsValidIndex(TargetContainerIndex)) return false;

	FInvContainer& Dst = TargetComponent->Containers[TargetContainerIndex];

	// 1) Collect item ids that already exist in destination
	TSet<FPrimaryAssetId> ExistingIds;
	for (const FInv_InventoryEntry& E : Dst.GetEntries())
	{
		ExistingIds.Add(E.GetItemId());
	}
	if (ExistingIds.Num() == 0)
	{
		// Nothing in destination — don't introduce new types
		return false;
	}

	// 2) Iterate all our source containers
	for (int32 SrcIdx = 0; SrcIdx < Containers.Num(); ++SrcIdx)
	{
		if (!Containers.IsValidIndex(SrcIdx)) continue;
		FInvContainer& Src = Containers[SrcIdx];

		// Optional rule: within same component block moves between identical types
		if (TargetComponent == this && Src.Type == Dst.Type)
		{
			continue;
		}

		// Copy candidate instance ids up-front to avoid iterator invalidation on remove
		TArray<FGuid> CandidateInstances;
		CandidateInstances.Reserve(Src.GetEntries().Num());
		for (const FInv_InventoryEntry& E : Src.GetEntries())
		{
			if (ExistingIds.Contains(E.GetItemId()))
			{
				CandidateInstances.Add(E.GetInstanceId());
			}
		}

		for (const FGuid& InstanceId : CandidateInstances)
		{
			const int32 Index = Src.FindIndexByInstance(InstanceId);
			if (Index == INDEX_NONE) continue;
			const FInv_InventoryEntry SrcEntry = Src.GetEntries()[Index];

			// Respect destination allowed items
			if (!Dst.IsItemAllowed(SrcEntry.GetItemType())) continue;

			// Resolve MaxStack from definition
			URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(SrcEntry.GetItemId());
			if (!Def) continue;
			const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
			const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

			int32 Remaining = SrcEntry.GetStack();
			if (Remaining <= 0) continue;

			FGuid NewInstanceId = InstanceId;
			int32 Added = 0;
			Dst.AddOrStack(SrcEntry.GetItemId(), SrcEntry.GetItemType(), MaxStack, Remaining, NewInstanceId, Added);
			if (Added <= 0) continue;

			int32 Removed = 0;
			Src.RemoveByInstance(InstanceId, Added, Removed);
			OutTotalMoved += FMath::Min(Added, Removed);
		}
	}

	return OutTotalMoved > 0;
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

void URpg_ContainerComponent::ServerAutoDepositMatchingTo_Implementation(URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex)
{
	int32 DummyMoved = 0;
	AutoDepositMatchingTo(TargetComponent, TargetContainerIndex, DummyMoved);
}
