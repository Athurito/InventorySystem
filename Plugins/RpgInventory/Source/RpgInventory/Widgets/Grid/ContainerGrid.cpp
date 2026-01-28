#include "ContainerGrid.h"

#include "GameFramework/PlayerState.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/Widgets/GridSlots/ContainerSlotButton.h"
#include "View/MVVMView.h"
#include "RpgInventory/Widgets/Mvvm/InventorySelectionViewModel.h"

void UContainerGrid::ActivateWidget()
{
	UE_LOG(LogTemp, Log, TEXT("[DEBUG_LOG] UContainerGrid::ActivateWidget called for %s"), *GetName());
	
	if (const UMVVMView* View = GetExtension<UMVVMView>())
	{
		// Wir suchen das ViewModel im MVVM View über den konfigurierten Namen
		UInventorySelectionViewModel* VM = Cast<UInventorySelectionViewModel>(View->GetViewModel(ViewModelName).GetInterface());
		
		// Fallback auf den Klassennamen, falls der konfigurierte Name nicht greift
		if (!VM)
		{
			VM = Cast<UInventorySelectionViewModel>(View->GetViewModel(FName("InventorySelectionViewModel")).GetInterface());
		}

		if (VM)
		{
			UInventoryManagerComponent* Manager = ResolveManagerForViewModel();
			int32 Index = ResolveContainerIndexForViewModel();
			UE_LOG(LogTemp, Log, TEXT("[DEBUG_LOG] Initializing VM with Manager: %s, Index: %d"), Manager ? *Manager->GetName() : TEXT("null"), Index);
			VM->InitializeFromManager(Manager, Index);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[DEBUG_LOG] ViewModel '%s' not found in View extension!"), *ViewModelName.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DEBUG_LOG] MVVMView extension not found on widget %s"), *GetName());
	}
}

void UContainerGrid::DeactivateWidget()
{
	if (const UMVVMView* View = GetExtension<UMVVMView>())
	{
		UInventorySelectionViewModel* VM = Cast<UInventorySelectionViewModel>(View->GetViewModel(ViewModelName).GetInterface());
		if (!VM)
		{
			VM = Cast<UInventorySelectionViewModel>(View->GetViewModel(FName("InventorySelectionViewModel")).GetInterface());
		}

		if (VM)
		{
			VM->InitializeFromManager(nullptr, -1);
		}
	}
}

void UContainerGrid::NativeConstruct()
{
	Super::NativeConstruct();
}

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
