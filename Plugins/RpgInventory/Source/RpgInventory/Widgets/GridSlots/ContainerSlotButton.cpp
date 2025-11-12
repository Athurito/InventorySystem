// Fill out your copyright notice in the Description page of Project Settings.


#include "ContainerSlotButton.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "RpgInventory/InventoryManagement/Components/Rpg_ContainerComponent.h"

void UContainerSlotButton::UpdateText() const
{
	if (StackCount <= 0)
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Text_StackCount->SetVisibility(ESlateVisibility::Visible);
	Text_StackCount->SetText(FText::FromString(FString::FromInt(StackCount)));
}

void UContainerSlotButton::UpdateIcon(UTexture2D* Icon) const
{
	if (!Icon)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Image_Icon->SetVisibility(ESlateVisibility::Visible);
	Image_Icon->SetBrushFromTexture(Icon);
}

URpg_ContainerComponent* UContainerSlotButton::GetOwningContainerComponent() const
{
	return OwningContainerComponent.Get();
}

void UContainerSlotButton::InitializeSlotContext(URpg_ContainerComponent* InComponent, int32 InContainerIndex)
{
	OwningContainerComponent = InComponent; OwningContainerIndex = InContainerIndex;
	
	if (OwningContainerComponent.IsValid())
	{
		OwningContainerComponent->OnInventorySlotChanged.AddDynamic(this, &UContainerSlotButton::HandleSlotChanged);
	}
}

void UContainerSlotButton::HandleSlotChanged(int32 ChangedSlotIndex)
{
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

	Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
	Image_Icon->SetBrushFromTexture(nullptr);

	Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	Text_StackCount->SetText(FText());
}
