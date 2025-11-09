// Fill out your copyright notice in the Description page of Project Settings.


#include "ContainerSlotButton.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

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
