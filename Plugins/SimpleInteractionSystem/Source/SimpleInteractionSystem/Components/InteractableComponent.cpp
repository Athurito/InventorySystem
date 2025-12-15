// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractableComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Components/ArrowComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "SimpleInteractionSystem/Data/InteractionDefinition.h"
#include "SimpleInteractionSystem/Interface/InteractionWidgetBindable.h"


UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;

	// Only data/state should replicate; UI never replicates.
	SetIsReplicatedByDefault(true);
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	AnchorArrow = FindOrCreateAnchorArrow();
	EnsureWidgetCreated();

	if (WidgetComponent)
	{
		WidgetComponent->SetVisibility(false, true);
	}
}

void UInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FocusedLocalPC = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInteractableComponent, bEnabled);
}

bool UInteractableComponent::CanInteract_Implementation(AActor* Interactor) const
{
	if (!bEnabled || !Definition || !Interactor)
	{
		return false;
	}

	// If you use tag requirements, we validate them via ASC on the Interactor
	const bool bHasReq = !Definition->RequiredInteractorTags.IsEmpty();
	const bool bHasBlocked = !Definition->BlockedInteractorTags.IsEmpty();

	if (bHasReq || bHasBlocked)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Interactor);
		if (!ASC)
		{
			// No ASC but requirements exist => fail (keeps behavior deterministic)
			return false;
		}

		if (bHasReq && !ASC->HasAllMatchingGameplayTags(Definition->RequiredInteractorTags))
		{
			return false;
		}

		if (bHasBlocked && ASC->HasAnyMatchingGameplayTags(Definition->BlockedInteractorTags))
		{
			return false;
		}
	}

	return true;
}

void UInteractableComponent::OnInteractionExecuted_Implementation(AActor* Interactor, FGameplayTag InteractionTag)
{
	// Default no-op. Override in BP for Door/Chest/etc.
}

UArrowComponent* UInteractableComponent::FindOrCreateAnchorArrow()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// 1) Find by name
	for (UActorComponent* C : Owner->GetComponents())
	{
		if (UArrowComponent* Arrow = Cast<UArrowComponent>(C))
		{
			if (Arrow->GetFName() == AnchorArrowName)
			{
				return Arrow;
			}
		}
	}

	// 2) Fallback: first arrow
	if (UArrowComponent* AnyArrow = Owner->FindComponentByClass<UArrowComponent>())
	{
		return AnyArrow;
	}

	// 3) Create a fallback arrow (plugin-friendly)
	UArrowComponent* NewArrow = NewObject<UArrowComponent>(Owner, AnchorArrowName);
	NewArrow->SetupAttachment(Owner->GetRootComponent());
	NewArrow->RegisterComponent();
	NewArrow->SetRelativeLocation(FVector(0, 0, 80.f));
	return NewArrow;
}

void UInteractableComponent::EnsureWidgetCreated()
{
	if (WidgetComponent || !GetOwner())
	{
		return;
	}

	if (!WidgetClass)
	{
		// No widget configured => nothing to create
		return;
	}

	WidgetComponent = NewObject<UWidgetComponent>(GetOwner(), TEXT("InteractionWidgetComponent"));
	WidgetComponent->RegisterComponent();

	if (!AnchorArrow)
	{
		AnchorArrow = FindOrCreateAnchorArrow();
	}

	// Attach to Arrow anchor (preferred)
	if (AnchorArrow)
	{
		WidgetComponent->AttachToComponent(AnchorArrow, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else if (GetOwner()->GetRootComponent())
	{
		WidgetComponent->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	}

	// "NiceShadow-like": Screen Space floating prompt
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);

	WidgetComponent->SetDrawSize(FIntPoint(static_cast<int32>(DrawSize.X), static_cast<int32>(DrawSize.Y)));
	WidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	WidgetComponent->SetTwoSided(true);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WidgetComponent->SetWidgetClass(WidgetClass);
	WidgetComponent->SetVisibility(false, true);
}

void UInteractableComponent::BindWidgetIfPossible()
{
	if (!WidgetComponent)
	{
		return;
	}

	UUserWidget* UW = WidgetComponent->GetUserWidgetObject();
	if (!UW)
	{
		return;
	}

	// Allow any widget to implement the binding interface (CommonUI widgets included)
	if (UW->GetClass()->ImplementsInterface(UInteractionWidgetBindable::StaticClass()))
	{
		IInteractionWidgetBindable::Execute_BindInteractable(UW, this);
	}
}

void UInteractableComponent::SetLocalFocused(bool bFocused, APlayerController* LocalPC)
{
	bIsLocallyFocused = bFocused;
	FocusedLocalPC = bFocused ? LocalPC : nullptr;

	if (!WidgetComponent)
	{
		EnsureWidgetCreated();
	}

	if (!WidgetComponent)
	{
		return;
	}

	// Critical for CommonUI / correct LocalPlayer context + glyphs:
	if (bFocused && LocalPC && LocalPC->GetLocalPlayer())
	{
		WidgetComponent->SetOwnerPlayer(LocalPC->GetLocalPlayer());
	}

	WidgetComponent->SetVisibility(bFocused, true);

	if (bFocused)
	{
		BindWidgetIfPossible();
	}
}

void UInteractableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
	Super::TickComponent(DeltaTime, TickType, ThisTick);

	if (!bIsLocallyFocused || !bBillboardToLocalCamera || !WidgetComponent)
	{
		return;
	}

	APlayerController* PC = FocusedLocalPC.Get();
	if (!PC)
	{
		return;
	}

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector WidgetLoc = WidgetComponent->GetComponentLocation();
	const FRotator LookAt = (CamLoc - WidgetLoc).Rotation();
	WidgetComponent->SetWorldRotation(LookAt);
}

