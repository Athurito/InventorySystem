// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemComponent.h"

#include "Net/UnrealNetwork.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Stackable.h"

void UInventoryItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemId);
}

int32 UInventoryItemComponent::GetMaxStackSize() const
{
	if (const UInventoryItemDefinition* Def = GetItemDefinition())
	{
		if (UInventoryFragment_Stackable* StackableFragment = Cast<UInventoryFragment_Stackable>(Def->FindFragmentByClass(UInventoryFragment_Stackable::StaticClass())))
		{
			return StackableFragment->GetMaxStackSize();
		}
	}
	return 1;
}


void UInventoryItemComponent::InitItemByDefinition(UInventoryItemDefinition* Definition)
{
	check(GetOwner() && GetOwner()->HasAuthority());

	ItemDefinition = Definition;
	ItemId   = Definition ? Definition->GetPrimaryAssetId() : FPrimaryAssetId();

	InitRuntimeFromDefinition(Definition);
}

void UInventoryItemComponent::InitItemById(FPrimaryAssetId Id)
{
	check(GetOwner() && GetOwner()->HasAuthority());

	ItemId = Id;
	ItemDefinition = nullptr;

	// Optional: sofort laden, damit Stack initialisiert werden kann
	if (ItemId.IsValid())
	{
		FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(ItemId);
		if (Path.IsValid())
		{
			if (UObject* Obj = Path.TryLoad())
			{
				ItemDefinition = Cast<UInventoryItemDefinition>(Obj);
			}
		}
	}
	InitRuntimeFromDefinition(ItemDefinition.Get());
}

void UInventoryItemComponent::InitItemBySoft(TSoftObjectPtr<UInventoryItemDefinition> Soft)
{
	check(GetOwner() && GetOwner()->HasAuthority());
	UInventoryItemDefinition* Def = Soft.IsValid() ? Soft.Get() : Soft.LoadSynchronous();
	InitItemByDefinition(Def);
}

void UInventoryItemComponent::OnRep_ItemId()
{
	ItemDefinition = nullptr;

	if (!ItemId.IsValid()) return;

	FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(ItemId);
	if (Path.IsValid())
	{
		if (UObject* Obj = Path.TryLoad()) // für kleine DataAssets ok; sonst async
		{
			ItemDefinition = Cast<UInventoryItemDefinition>(Obj);
			// Initialize/refresh runtime data on clients when definition arrives
			InitRuntimeFromDefinition(ItemDefinition.Get());
		}
	}
}

void UInventoryItemComponent::OnRep_RuntimeData()
{
	// UI/FX-Refresh (Widgets, Sounds etc.) could be triggered here if needed
}

void UInventoryItemComponent::InitRuntimeFromDefinition(const UInventoryItemDefinition* Def)
{
	if (Def)
	{
		// Initialize Stackable runtime data if definition has the fragment
		if (const FInventoryFragment_Stackable* Stack = Def->GetFragmentOfTypeWithTag<FInventoryFragment_Stackable>(FragmentTags::StackableFragment))
		{
			const int32 Max = FMath::Max(1, Stack->GetMaxStackSize());

			RuntimeData.Modify<FStackableRuntimeData>(FragmentTags::StackableFragment,
				[&](FStackableRuntimeData& D)
				{
					// Mindestwert 1 beim Initialisieren (dein bisheriges Verhalten)
					if (D.CurrentStackCount <= 0)
					{
						D.CurrentStackCount = 1;
					}
					D.CurrentStackCount = FMath::Clamp(D.CurrentStackCount, 1, Max);
				});
		}
	}
}

bool UInventoryItemComponent::Consume(APawn* Instigator)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		// auf Clients nie State ändern
		return false;
	}
	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("Consume failed: ItemData is null"));
		return false;
	}

	const FConsumableFragment* Consumable = ItemDefinition->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment);
	if (!Consumable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Consume failed: No ConsumableFragment on item"));
		return false;
	}

	// Check runtime stack if required
	if (Consumable->bReduceStack)
	{
		const int32 Max = GetMaxStackSize();

		// Lesen: ohne Anlegen
		const FStackableRuntimeData* StackConst =
			RuntimeData.FindConst<FStackableRuntimeData>(FragmentTags::StackableFragment);

		int32 Current = StackConst ? StackConst->CurrentStackCount : 1;

		if (Current < Consumable->QuantityPerUse)
		{
			UE_LOG(LogTemp, Warning, TEXT("Consume failed: Not enough stack. Have %d, need %d"),
				   Current, Consumable->QuantityPerUse);
			return false;
		}

		const int32 NewCurrent =
			FMath::Clamp(Current - Consumable->QuantityPerUse, 0, Max);

		// Schreiben: legt bei Bedarf an und markiert automatisch dirty
		RuntimeData.Modify<FStackableRuntimeData>(FragmentTags::StackableFragment,
			[&](FStackableRuntimeData& D)
			{
				D.CurrentStackCount = NewCurrent;
			});
	}

	// Durability not implemented yet in this module; log if requested
	if (Consumable->bReduceDurability)
	{
		UE_LOG(LogTemp, Warning, TEXT("Consume: bReduceDurability is true but durability system not implemented. Skipping wear."));
	}

	// Helper to resolve an ASC from Instigator/Controller/PlayerState/Owner
	auto ResolveASC = [](APawn* InInstigator, AActor* InOwner) -> UAbilitySystemComponent*
	{
		if (InInstigator)
		{
			// Pawn implements ASI
			if (const IAbilitySystemInterface* ASIInst = Cast<IAbilitySystemInterface>(InInstigator))
			{
				if (UAbilitySystemComponent* C = ASIInst->GetAbilitySystemComponent()) return C;
			}
			// PlayerState usually holds ASC
			if (APlayerState* PS = InInstigator->GetPlayerState())
			{
				if (const IAbilitySystemInterface* ASIPS = Cast<IAbilitySystemInterface>(PS))
				{
					if (UAbilitySystemComponent* C = ASIPS->GetAbilitySystemComponent()) return C;
				}
			}
			// Controller may also implement
			if (AController* Cntr = InInstigator->GetController())
			{
				if (const IAbilitySystemInterface* ASIC = Cast<IAbilitySystemInterface>(Cntr))
				{
					if (UAbilitySystemComponent* C = ASIC->GetAbilitySystemComponent()) return C;
				}
			}
		}
		// Fallback to owner of item component
		if (InOwner)
		{
			if (const IAbilitySystemInterface* ASIOwner = Cast<IAbilitySystemInterface>(InOwner))
			{
				if (UAbilitySystemComponent* C = ASIOwner->GetAbilitySystemComponent()) return C;
			}
		}
		return nullptr;
	};
	return true;
}


FInteractDisplayData UInventoryItemComponent::GetDisplayData_Implementation() const
{
	if (!bEnabled || !ItemDefinition) return FInteractDisplayData();

	FInteractDisplayData Data;
	
	Data.ActionText = ItemDefinition->GetInteractionText();
	
	return Data;
}

bool UInventoryItemComponent::CanInteract_Implementation(APawn* Instigator) const
{
	if (!bEnabled || !GetOwner()) return false;
	if (!Instigator) return false;

	const float Dist = FVector::Dist(Instigator->GetActorLocation(), GetOwner()->GetActorLocation());
	return Dist <= MaxUseDistance;
}

void UInventoryItemComponent::Interact_Implementation(APawn* Instigator)
{
	URpg_ContainerComponent* InventoryComponent = nullptr;
	
	InventoryComponent = UInventoryStatics::ResolveInventoryFromInstigator(Instigator);
	
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Interact: NO InventoryComponent found anywhere!"));
		return;
	}
	
	const UInventoryItemDefinition* Def = GetItemDefinition();
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: No ItemDefinition, aborting"));
		return;
	}

	// 1) If item is consumable, respect policy
	if (const FConsumableFragment* Cons = Def->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment))
	{
		switch (Cons->UseAvailability)
		{
			case EUseAvailability::WorldOnly:
			case EUseAvailability::WorldOrInventory:
			{
				InventoryComponent->TryUseWorldItem(this, FMath::Max(1, Cons->QuantityPerUse));
				return;
			}
			case EUseAvailability::InventoryOnly:
			{
				// Not usable directly in world; fall through to pickup
				break;
			}
			case EUseAvailability::PickupThenUseIfWorld:
			{
				// We will pick up below; after successful pickup we may auto use
				break;
			}
		}
	}

	// 2) Otherwise: attempt to pick up into an appropriate container
	const FGameplayTag ItemType = Def->GetItemType();
	int32 TargetContainerIdx = INDEX_NONE;
	// Find first container that allows this item type
	for (int32 i = 0; i < InventoryComponent->Containers.Num(); ++i)
	{
		if (InventoryComponent->Containers[i].IsItemAllowed(ItemType))
		{
			TargetContainerIdx = i;
			break;
		}
	}
	if (TargetContainerIdx == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: No accepting container for item type %s"), *ItemType.ToString());
		return;
	}

	// Determine quantity to pick up
	int32 QuantityToAdd = 1;
	if (Def->GetFragmentOfTypeWithTag<FInventoryFragment_Stackable>(FragmentTags::StackableFragment))
	{
		QuantityToAdd = FMath::Max(1, GetCurrentStackCount());
	}

	int32 OutAdded = 0; FGuid OutInstanceId;
	const bool bRequested = InventoryComponent->AddItemToContainer(TargetContainerIdx, this, QuantityToAdd, OutAdded, OutInstanceId);

	// If we are on the server, reduce or destroy the world item based on how much was actually added
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (OutAdded > 0)
		{
			AActor* OwnerActor = GetOwner();
			const bool bIsStackable =
				(Def->GetFragmentOfTypeWithTag<FInventoryFragment_Stackable>(FragmentTags::StackableFragment) != nullptr);

			if (bIsStackable)
			{
				// Aktuellen Wert nur lesen (legt nichts an)
				const FStackableRuntimeData* StackConst =
					RuntimeData.FindConst<FStackableRuntimeData>(FragmentTags::StackableFragment);

				// Fallback: wenn noch kein RuntimeData existiert, verhalte dich wie „1 vor Abzug“
				const int32 Current = StackConst ? StackConst->CurrentStackCount : 1;
				const int32 NewCount = FMath::Max(0, Current - OutAdded);

				bool bBecameZero = false;
				// Schreiben: legt bei Bedarf an + markiert automatisch dirty
				RuntimeData.Modify<FStackableRuntimeData>(FragmentTags::StackableFragment,
					[&](FStackableRuntimeData& D)
					{
						D.CurrentStackCount = NewCount;
						bBecameZero = (D.CurrentStackCount <= 0);
					});

				if (bBecameZero && OwnerActor)
				{
					OwnerActor->Destroy();
				}
			}
			else
			{
				// Non-stackable: sobald etwas entnommen wurde, Actor zerstören
				if (OwnerActor)
				{
					OwnerActor->Destroy();
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Interact: AddItemToContainer added nothing (full or disallowed)"));
		}
	}
	else
	{
		// On clients we just requested via RPC; the server will replicate inventory and possibly destroy the world item.
		UE_LOG(LogTemp, Verbose, TEXT("Interact: Pickup requested (client), waiting for replication"));
	}
}

void UInventoryItemComponent::BeginPlay()
{
	Super::BeginPlay();

	// Level-platziert: Server übernimmt InitialDefinition einmalig
	if (GetOwner() && GetOwner()->HasAuthority() && InitialDefinition.IsValid())
	{
		UInventoryItemDefinition* Def = InitialDefinition.Get();
		if (!Def) Def = InitialDefinition.LoadSynchronous();
		InitItemByDefinition(Def);
	}
}

