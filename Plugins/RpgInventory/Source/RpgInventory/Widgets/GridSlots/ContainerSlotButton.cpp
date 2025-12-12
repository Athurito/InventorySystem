// Fill out your copyright notice in the Description page of Project Settings.


#include "ContainerSlotButton.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"


UInventoryManagerComponent* UContainerSlotButton::GetOwningContainerComponent() const
{
	return OwningContainerComponent.Get();
}
