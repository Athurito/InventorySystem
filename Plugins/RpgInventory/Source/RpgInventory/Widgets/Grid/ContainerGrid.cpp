#include "ContainerGrid.h"

#include "GameFramework/PlayerState.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/Widgets/GridSlots/ContainerSlotButton.h"


void UContainerGrid::NativeDestruct()
{
	Super::NativeDestruct();
}

UInventoryManagerComponent* UContainerGrid::ResolveManagerForViewModel() const
{
	if (SourceType == EInventorySourceType::Storage)
	{
		return StorageManager.Get();
	}

	// Player-Manager wie in deinem ursprünglichen Blueprint:
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (APlayerState* PS = PC->PlayerState)
		{
			return PS->FindComponentByClass<UInventoryManagerComponent>();
		}
	}
	return nullptr;
}

int32 UContainerGrid::ResolveContainerIndexForViewModel() const
{
	return (SourceType == EInventorySourceType::Storage)
		? StorageContainerIndex
		: PlayerContainerIndex;
}
