// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Interact/Widget/InteractPromptWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "InventoryManagement/Interact/Data/InteractableDataAsset.h"

void UInteractPromptWidget::SetPromptData(const FInteractDisplayData& Data)
{
	if (TitleText)
	{
		TitleText->SetText(Data.Title);
	}
	if (ActionText)
	{
		ActionText->SetText(Data.ActionText);
	}

	// Resolve icon: might be already loaded, otherwise sync-load as fallback.
	UTexture2D* IconTex = nullptr;
	if (!Data.Icon.IsNull())
	{
		IconTex = Data.Icon.IsValid() ? Data.Icon.Get() : Data.Icon.LoadSynchronous();
	}
	if (IconImage)
	{
		if (IconTex)
		{
			IconImage->SetBrushFromTexture(IconTex, true);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetBrushFromTexture(nullptr);
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UInteractPromptWidget::SetPromptVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
