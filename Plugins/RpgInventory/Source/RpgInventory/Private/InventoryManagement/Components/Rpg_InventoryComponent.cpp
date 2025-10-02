// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Rpg_InventoryComponent.h"

#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Items/Components/Rpg_ItemComponent.h"
#include "Items/ItemData.h"
#include "Items/Fragments/ItemFragment.h"

// Sets default values for this component's properties
URpg_InventoryComponent::URpg_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// Client/UI entry point
void URpg_InventoryComponent::TryConsumeItem(URpg_ItemComponent* ItemComponent, int32 Quantity)
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

// Server authoritative execution
void URpg_InventoryComponent::ServerConsumeItem_Implementation(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	InternalConsume(ItemComponent, Quantity);
}

bool URpg_InventoryComponent::InternalConsume(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	if (!ensure(ItemComponent))
	{
		return false;
	}

	// Validate item belongs to us or is usable; minimal check for now
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (!InstigatorPawn)
	{
		// Fallback: try the item's owner as instigator pawn
		InstigatorPawn = Cast<APawn>(ItemComponent->GetOwner());
	}

	UItemData* ItemData = ItemComponent->GetItemData();
	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("InternalConsume: ItemData is null"));
		return false;
	}

	const FConsumableFragment* Consumable = ItemData->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment);
	if (!Consumable)
	{
		UE_LOG(LogTemp, Warning, TEXT("InternalConsume: Item not consumable"));
		return false;
	}

	// We support consuming N times by looping Quantity
	int32 ConsumedTotal = 0;
	for (int32 i = 0; i < Quantity; ++i)
	{
		if (!ItemComponent->Consume(InstigatorPawn))
		{
			break;
		}
		ConsumedTotal++;
	}

	if (ConsumedTotal > 0)
	{
		OnItemConsumed.Broadcast(ItemComponent, ConsumedTotal);

		// If stackable and now zero, we could handle removal (minimal: log)
		if (FStackableFragment* Stack = ItemData->GetFragmentOfTypeMutable<FStackableFragment>())
		{
			if (Stack->GetStackCount() <= 0)
			{
				UE_LOG(LogTemp, Log, TEXT("InternalConsume: Stack depleted; item should be removed from inventory"));
			}
		}
	}

	return ConsumedTotal > 0;
}

// Called when the game starts
void URpg_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

