// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Rpg_InventoryComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "Items/Components/Rpg_ItemComponent.h"
#include "Items/Rpg_ItemDefinition.h"
#include "Items/Fragments/ConsumableFragment.h"

URpg_InventoryComponent::URpg_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URpg_InventoryComponent::TryConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity)
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

void URpg_InventoryComponent::ServerConsumeItem_Implementation(URpg_ItemComponent* ItemComponent, const int32 Quantity)
{
	if (!ItemComponent || !IsValid(ItemComponent) || !IsValid(ItemComponent->GetOwner())) return;
	InternalConsume(ItemComponent, Quantity);
}

bool URpg_InventoryComponent::InternalConsume(URpg_ItemComponent* ItemComponent, int32 const Quantity) const
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

APawn* URpg_InventoryComponent::ResolveInstigator(const URpg_ItemComponent* ItemComponent) const
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

