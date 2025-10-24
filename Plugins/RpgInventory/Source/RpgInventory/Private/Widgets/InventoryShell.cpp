// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/InventoryShell.h"

#include "Components/Overlay.h"
#include "Widgets/Grid/ContainerGrid.h"
#include "InventoryManagement/Components/Rpg_ContainerComponent.h"

void UInventoryShell::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UInventoryShell::NativeOnActivated()
{
	Super::NativeOnActivated();
	ApplyPlayerBinding();
	ApplyContextWidget();
}

void UInventoryShell::InitializeShell(URpg_ContainerComponent* PlayerContainer, int32 InPlayerContainerIndex,
	URpg_ContainerComponent* ContextContainer, int32 InContextContainerIndex, UWidget* ContextWidget)
{
	PlayerContainerComponent = PlayerContainer;
	PlayerContainerIndex = InPlayerContainerIndex;
	ContextContainerComponent = ContextContainer;
	ContextContainerIndex = InContextContainerIndex;
	ActiveContextWidget = ContextWidget;

	ApplyPlayerBinding();
	ApplyContextWidget();
}

void UInventoryShell::InitializeShellPlayerOnly(URpg_ContainerComponent* PlayerContainer, int32 InPlayerContainerIndex)
{
	PlayerContainerComponent = PlayerContainer;
	PlayerContainerIndex = InPlayerContainerIndex;
	ActiveContextWidget = nullptr; // Kontext später setzen
	ApplyPlayerBinding();
	ApplyContextWidget();
}

void UInventoryShell::SetContextWidget(UWidget* InWidget)
{
	ActiveContextWidget = InWidget;
	ApplyContextWidget();
}

void UInventoryShell::ApplyPlayerBinding()
{
	if (PlayerInventoryGrid && PlayerContainerComponent.IsValid())
	{
		PlayerInventoryGrid->BindToContainer(PlayerContainerComponent.Get(), PlayerContainerIndex);
	}
}

void UInventoryShell::ApplyContextWidget()
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
