// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTabButton.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"

void UInventoryTabButton::SetLabelAndIcon(const FText& InText, UTexture2D* InIcon)
{
	if (Label) Label->SetText(InText);
	if (Icon)  Icon->SetBrushFromTexture(InIcon);
}
