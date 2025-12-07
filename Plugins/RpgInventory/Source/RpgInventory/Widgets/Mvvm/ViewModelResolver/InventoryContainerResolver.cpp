// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryContainerResolver.h"

#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/Widgets/Grid/ContainerGrid.h"
#include "RpgInventory/Widgets/Mvvm/InventorySelectionViewModel.h"

UObject* UInventoryContainerResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (!ExpectedType || !ExpectedType->IsChildOf(UInventorySelectionViewModel::StaticClass()))
	{
		return nullptr;
	}

	const UContainerGrid* Grid = Cast<UContainerGrid>(UserWidget);
	if (!Grid)
	{
		return nullptr;
	}

	UInventoryManagerComponent* Manager = Grid->ResolveManagerForViewModel();
	const int32 ContainerIndex = Grid->ResolveContainerIndexForViewModel();

	if (!Manager || ContainerIndex == INDEX_NONE)
	{
		return nullptr;
	}

	UInventorySelectionViewModel* VM =
		NewObject<UInventorySelectionViewModel>(
			const_cast<UContainerGrid*>(Grid),
			ExpectedType
		);

	VM->InitializeFromManager(Manager, ContainerIndex);

	return VM;
}
