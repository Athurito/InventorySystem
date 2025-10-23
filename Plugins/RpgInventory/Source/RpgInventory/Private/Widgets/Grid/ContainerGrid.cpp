// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Grid/ContainerGrid.h"

#include "InventoryManagement/Utils/InventoryStatics.h"

void UContainerGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ContainerComponent = UInventoryStatics::GetContainerComponent(GetOwningPlayer());
	
	if (!ContainerComponent.IsValid()) return;
}
