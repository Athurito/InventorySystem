// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionManagement/Widget/InteractPromptWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "InteractionManagement/Data/InteractableDataAsset.h"

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
