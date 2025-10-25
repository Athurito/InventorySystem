// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/GridSlots/ContainerSlotButton.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UContainerSlotButton::UpdateText() const
{
	Text_StackCount->SetText(FText::FromString(FString::FromInt(StackCount)));
}

void UContainerSlotButton::UpdateIcon(UTexture2D* Icon) const
{
	Image_Icon->SetBrushFromTexture(Icon);
}
