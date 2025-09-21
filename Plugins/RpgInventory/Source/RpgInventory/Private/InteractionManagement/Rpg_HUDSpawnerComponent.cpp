// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionManagement/Rpg_HUDSpawnerComponent.h"

#include "Blueprint/UserWidget.h"
#include "InteractionManagement/Rpg_InteractionComponent.h"
#include "InteractionManagement/Widget/Rpg_HUDWidget.h"

URpg_HUDSpawnerComponent::URpg_HUDSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpg_HUDSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedPC = Cast<APlayerController>(GetOwner());
	if (!CachedPC.IsValid() || !CachedPC->IsLocalController())
	{
		// Nur auf dem lokalen Controller UI erzeugen
		return;
	}

	// HUD erstellen (falls konfiguriert)
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<URpg_HUDWidget>(CachedPC.Get(), HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(HUDZOrder);
		}
	}

	// Einmalig an die InteractionComponent am Controller binden
	BindHUDToControllerInteraction();
}

void URpg_HUDSpawnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void URpg_HUDSpawnerComponent::BindHUDToControllerInteraction()
{
	if (!HUDWidget || !CachedPC.IsValid())
		return;

	URpg_InteractionComponent* Interaction =
		CachedPC->FindComponentByClass<URpg_InteractionComponent>();

	// HUD kennt die Bind/Unbind-Logik intern (RemoveDynamic/ AddDynamic auf alter/neuer Comp)
	HUDWidget->BindToInteraction(Interaction);
}

