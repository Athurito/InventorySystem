// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Interact/Widget/InteractPromptWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "InventoryManagement/Interact/Data/InteractableDataAsset.h"

void UInteractPromptWidget::SetPromptData(const FInteractDisplayData& Data)
{
	if (Text_Message)
	{
		Text_Message->SetText(Data.ActionText);
	}
}

void UInteractPromptWidget::SetPromptVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
