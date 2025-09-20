#include "Inventory/World/RPGWorldPickup.h"
#include "Inventory/World/ItemPickupComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

ARPGWorldPickup::ARPGWorldPickup()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	PickupComponent = CreateDefaultSubobject<UItemPickupComponent>(TEXT("ItemPickup"));
}

void ARPGWorldPickup::Initialize(UItemDefinition* InDefinition, int32 InStackCount)
{
	if (ensure(PickupComponent))
	{
		PickupComponent->InitializeFromDefinition(InDefinition, InStackCount);
	}
}

FInteractDisplayData ARPGWorldPickup::GetDisplayData_Implementation() const
{
	FInteractDisplayData Data;
	if (PickupComponent)
	{
		Data.Title = FText::FromString(TEXT("Pickup"));
		Data.ActionText = FText::FromString(TEXT("Aufheben"));
	}
	return Data;
}

bool ARPGWorldPickup::CanInteract_Implementation(APawn* InInstigator) const
{
	return true;
}

void ARPGWorldPickup::Interact_Implementation(APawn* InInstigator)
{
	if (PickupComponent)
	{
		PickupComponent->TryPickup(InInstigator);
	}
}

void ARPGWorldPickup::BeginPlay()
{
	Super::BeginPlay();
}
