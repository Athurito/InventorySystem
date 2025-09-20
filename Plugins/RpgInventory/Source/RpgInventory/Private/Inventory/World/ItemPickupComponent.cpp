#include "Inventory/World/ItemPickupComponent.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/Runtime/RPGInventoryComponent.h"
#include "GameFramework/Pawn.h"

UItemPickupComponent::UItemPickupComponent()
{
	SetIsReplicatedByDefault(true);
}

void UItemPickupComponent::BeginPlay()
{
	Super::BeginPlay();
	UpdateVisuals();
}

void UItemPickupComponent::InitializeFromDefinition(UItemDefinition* InDefinition, int32 InStackCount)
{
	Definition = InDefinition;
	StackCount = FMath::Max(1, InStackCount);
	UpdateVisuals();
}

bool UItemPickupComponent::TryPickup(APawn* InstigatorPawn)
{
	if (!InstigatorPawn)
	{
		return false;
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (URPGInventoryComponent* Inv = InstigatorPawn->FindComponentByClass<URPGInventoryComponent>())
		{
			if (Definition && StackCount > 0 && Inv->AddItem(Definition, StackCount))
			{
				GetOwner()->Destroy();
				return true;
			}
		}
	}
	return false;
}

void UItemPickupComponent::OnRep_Data()
{
	UpdateVisuals();
}

void UItemPickupComponent::UpdateVisuals() const
{
	// TODO: read UItemFragment_Visual from Definition and update mesh/icon if needed
}

void UItemPickupComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UItemPickupComponent, Definition);
	DOREPLIFETIME(UItemPickupComponent, StackCount);
}
