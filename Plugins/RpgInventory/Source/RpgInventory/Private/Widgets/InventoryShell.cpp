// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/InventoryShell.h"

#include "CommonTabListWidgetBase.h"
#include "Components/Overlay.h"

void UInventoryShell::NativeOnActivated()
{
	Super::NativeOnActivated();
	ApplyContextWidget();
}

void UInventoryShell::SetContextWidget(UWidget* InWidget)
{
	ActiveContextWidget = InWidget;
	ApplyContextWidget();
}

void UInventoryShell::ApplyContextWidget() const
{
	if (!DynamicContentRoot)
	{
		return;
	}

	DynamicContentRoot->ClearChildren();
	if (ActiveContextWidget)
	{
		DynamicContentRoot->AddChild(ActiveContextWidget);
	}
}
