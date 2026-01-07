// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemComponent.h"

#include "Net/UnrealNetwork.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Controller.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Stackable.h"
#include "RpgInventory/InventoryManagement/Items/Fragments/InventoryFragment_Pickup.h"
#include "RpgInventory/InventoryManagement/Utils/InventoryStatics.h"

void UInventoryItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ItemId);
	DOREPLIFETIME(ThisClass, CurrentStackCount);
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

	// Initialmenge wird nun direkt über InitialStackCount gesteuert
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CurrentStackCount = InitialStackCount;
	}
}

void UInventoryItemComponent::InitItemById(FPrimaryAssetId Id)
{
	check(GetOwner() && GetOwner()->HasAuthority());
	
	if (UInventoryItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(Id))
	{
		InitItemByDefinition(Def);
	}
}

void UInventoryItemComponent::InitItemBySoft(TSoftObjectPtr<UInventoryItemDefinition> Soft)
{
	check(GetOwner() && GetOwner()->HasAuthority());
	UInventoryItemDefinition* Def = Soft.IsValid() ? Soft.Get() : Soft.LoadSynchronous();
	InitItemByDefinition(Def);
}

void UInventoryItemComponent::OnRep_ItemId()
{
	if (!ItemId.IsValid())
	{
		ItemDefinition = nullptr;
		return;
	}

	FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(ItemId);
	if (Path.IsValid())
	{
		if (UObject* Obj = Path.TryLoad())
		{
			ItemDefinition = Cast<UInventoryItemDefinition>(Obj);
		}
	}
}

FInteractDisplayData UInventoryItemComponent::GetDisplayData_Implementation() const
{
	FInteractDisplayData Data;
	if (!bEnabled) return Data;

	const UInventoryItemDefinition* Def = GetItemDefinition();
	if (!Def) return Data;

	if (const UInventoryFragment_Pickup* PickupFragment = Cast<UInventoryFragment_Pickup>(Def->FindFragmentByClass(UInventoryFragment_Pickup::StaticClass())))
	{
		Data.ActionText = PickupFragment->GetInteractionText();
	}
	else
	{
		Data.ActionText = FText::Format(NSLOCTEXT("Inventory", "PickupAction", "Pick up {0}"), FText::FromName(Def->GetFName()));
	}

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
	UInventoryManagerComponent* InventoryComponent = UInventoryStatics::ResolveInventoryFromInstigator(Instigator);
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Interact: NO InventoryComponent found!"));
		return;
	}
	
	const UInventoryItemDefinition* Def = GetItemDefinition();
	if (!Def) return;

	// Manager bitten, die Menge aufzunehmen (verteilt auf Stacks/Slots)
	int32 AddedCount = InventoryComponent->TryAddItemDefinition(const_cast<UInventoryItemDefinition*>(Def), CurrentStackCount);

	if (AddedCount > 0)
	{
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			CurrentStackCount -= AddedCount;
			
			if (CurrentStackCount <= 0)
			{
				GetOwner()->Destroy();
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Interact: Partial pickup. %d items remaining in world."), CurrentStackCount);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Interact: Inventory full, could not pick up anything"));
	}
}

void UInventoryItemComponent::BeginPlay()
{
	Super::BeginPlay();

	// Level-platziert: Server übernimmt InitialDefinition einmalig
	if (GetOwner() && GetOwner()->HasAuthority() && ItemDefinition.IsValid())
	{
		UInventoryItemDefinition* Def = ItemDefinition.Get();
		if (!Def) Def = ItemDefinition.LoadSynchronous();
		InitItemByDefinition(Def);
	}
}

