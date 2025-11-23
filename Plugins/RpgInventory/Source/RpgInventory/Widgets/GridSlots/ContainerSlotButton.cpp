// Fill out your copyright notice in the Description page of Project Settings.


#include "ContainerSlotButton.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"


UInventoryManagerComponent* UContainerSlotButton::GetOwningContainerComponent() const
{
	return OwningContainerComponent.Get();
}

void UContainerSlotButton::InitializeSlotContext(UInventoryManagerComponent* InComponent, int32 InContainerIndex)
{
	OwningContainerComponent = InComponent; OwningContainerIndex = InContainerIndex;
	
	if (OwningContainerComponent.IsValid())
	{
		OwningContainerComponent->OnInventorySlotChanged.AddDynamic(this, &UContainerSlotButton::HandleSlotChanged);
	}
}

void UContainerSlotButton::HandleSlotChanged(int32 ChangedContainerIndex, int32 ChangedSlotIndex)
{
    // Filter: nur reagieren, wenn der Event für unseren Container und Slot ist
    if (ChangedContainerIndex != OwningContainerIndex)
    {
        return;
    }
    if (ChangedSlotIndex == SlotIndex)
    {
        RefreshSlot();
    }
}

void UContainerSlotButton::RefreshSlot()
{
	check(OwningContainerComponent.IsValid());
	
	UInventoryItemInstance* Item = OwningContainerComponent->GetItemInstanceInSlot(SlotIndex, OwningContainerIndex);

	if (!Item)
	{
		ClearSlot();
		return;
	}
}

void UContainerSlotButton::ClearSlot()
{
	InventoryItem = nullptr;
}
