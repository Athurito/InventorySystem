// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventorySystemPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "InventorySystem.h"
#include "Items/Components/Rpg_ItemComponent.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AInventorySystemPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogInventorySystem, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AInventorySystemPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}

	UEnhancedInputComponent* EIC = nullptr;
	if (!EIC) EIC = Cast<UEnhancedInputComponent>(InputComponent);

	if (EIC)
	{
		EIC->BindAction(InteractInputAction, ETriggerEvent::Started, this, &AInventorySystemPlayerController::SpawnTestItem);
	}
}

void AInventorySystemPlayerController::ServerSpawnTestItem_Implementation()
{
	if (!DropClass) return;
	FVector Location;
	FRotator SpawnRotation;
	
	// Prüfen, ob der Controller einen Pawn besitzt
	if (APawn* MyPawn = GetPawn())
	{
		Location = MyPawn->GetActorLocation() + MyPawn->GetActorForwardVector() * 200.f;
		SpawnRotation = MyPawn->GetActorRotation();
	}
	else
	{
		// Fallback (z. B. wenn du keinen Pawn besitzt)
		Location = FVector::ZeroVector;
		SpawnRotation = FRotator::ZeroRotator;
	}
	FTransform SpawnTransform(SpawnRotation, Location);
	
	AActor* Drop = GetWorld()->SpawnActor<AActor>(DropClass, SpawnTransform);
	if (auto* ItemComp = Drop->FindComponentByClass<URpg_ItemComponent>())
	{
		// Variante 1 – über SoftReference
		if (TestDefinition.IsValid() || !TestDefinition.ToSoftObjectPath().IsNull())
		{
			URpg_ItemDefinition* Def = TestDefinition.IsValid() ? TestDefinition.Get() : TestDefinition.LoadSynchronous();
			ItemComp->InitItemByDefinition(Def);
		}
		else
		{
			// Variante 2 – über PrimaryAssetId (automatisch erkannt)
			ItemComp->InitItemById(FPrimaryAssetId(TEXT("Item"), TEXT("DA_Potion")));
		}
	}
}

void AInventorySystemPlayerController::SpawnTestItem()
{
	if (HasAuthority())
	{
		ServerSpawnTestItem();
	}
	else
	{
		ServerSpawnTestItem();
	}
}
