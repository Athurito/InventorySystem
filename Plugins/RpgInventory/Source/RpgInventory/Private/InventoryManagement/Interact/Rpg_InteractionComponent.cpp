// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Interact/Rpg_InteractionComponent.h"

#include "InventoryManagement/Interact/Rpg_InteractableComponent.h"
#include "InventoryManagement/Interact/Interface/Interactable.h"
#include "InventoryManagement/Interact/Widget/InteractPromptWidget.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

URpg_InteractionComponent::URpg_InteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bAutoActivate = true;
}


void URpg_InteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Resolve owner pawn and player controller
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = nullptr;
	if (OwnerPawn)
	{
		PC = Cast<APlayerController>(OwnerPawn->GetController());
	}
	else if (APlayerController* OwnerPC = Cast<APlayerController>(GetOwner()))
	{
		PC = OwnerPC;
		OwnerPawn = OwnerPC->GetPawn();
	}

	// Create prompt widget on local player if a class is provided
	if (PromptWidgetClass && PC)
	{
		PromptWidget = CreateWidget<UInteractPromptWidget>(PC, PromptWidgetClass);
		if (PromptWidget)
		{
			PromptWidget->AddToViewport(PromptZOrder);
			PromptWidget->SetPromptVisible(false);
		}
	}

	// Try to auto-bind Enhanced Input action if configured
	if (InteractInputAction)
	{
		UEnhancedInputComponent* EIC = nullptr;
		if (OwnerPawn && OwnerPawn->InputComponent)
		{
			EIC = Cast<UEnhancedInputComponent>(OwnerPawn->InputComponent);
		}
		if (!EIC && PC && PC->InputComponent)
		{
			EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
		}
		if (EIC)
		{
			EIC->BindAction(InteractInputAction, ETriggerEvent::Started, this, &URpg_InteractionComponent::TryInteract);
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("URpg_InteractionComponent: EnhancedInputComponent not available to bind Interact."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("URpg_InteractionComponent: InteractInputAction is null; assign it in the component settings."));
	}
}

void URpg_InteractionComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	UpdateTrace();
}

UObject* URpg_InteractionComponent::FindInteractableOn(AActor* Actor, UPrimitiveComponent* HitComp) const
{
	if (!Actor) return nullptr;

	// 1) Actor selbst?
	if (Actor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
		return Actor;

	// 2) (selten) das getroffene Primitive als Interactable
	if (HitComp && HitComp->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
		return HitComp;

	// 3) Komponente am Actor
	if (UActorComponent* C = Actor->FindComponentByClass<URpg_InteractableComponent>())
		return C;

	// 4) Falls mehrere — nimm die erste, die das Interface hat
	TArray<UActorComponent*> All;
	Actor->GetComponents(All);
	for (UActorComponent* Comp : All)
	{
		if (Comp && Comp->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
			return Comp;
	}
	return nullptr;
}

void URpg_InteractionComponent::UpdateTrace()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = nullptr;
	if (OwnerPawn)
	{
		PC = Cast<APlayerController>(OwnerPawn->GetController());
	}
	else if (APlayerController* OwnerPC = Cast<APlayerController>(GetOwner()))
	{
		PC = OwnerPC;
		OwnerPawn = OwnerPC->GetPawn();
	}
	if (!PC) return;

	FVector ViewLoc; FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector Start = ViewLoc;
	const FVector End   = Start + ViewRot.Vector() * TraceDistance;

	FHitResult Hit;
	AActor* IgnoreActor = OwnerPawn ? static_cast<AActor*>(OwnerPawn) : Cast<AActor>(PC);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), /*bTraceComplex*/false, IgnoreActor);
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

	if (bDebug) DrawDebugLine(GetWorld(), Start, Hit.bBlockingHit ? Hit.ImpactPoint : End, Hit.bBlockingHit ? FColor::Green : FColor::Red, false, 0.f, 0, 0.5f);

	UObject* NewInteractable = nullptr;
	AActor*  NewActor = nullptr;

	if (Hit.bBlockingHit && Hit.GetActor())
	{
		NewInteractable = FindInteractableOn(Hit.GetActor(), Hit.GetComponent());
		NewActor = Hit.GetActor();
		// Optional: zusätzlich CanInteract früh checken, damit UI nur erscheint, wenn echt möglich
		if (NewInteractable && !IInteractable::Execute_CanInteract(NewInteractable, OwnerPawn))
		{
			NewInteractable = nullptr;
			NewActor = nullptr;
		}
	}

	if (CurrentInteractable.Get() != NewInteractable)
	{
		OnTargetChanged(NewInteractable, NewActor);
	}
}

void URpg_InteractionComponent::OnTargetChanged(UObject* NewInteractable, AActor* NewActor)
{
	CurrentInteractable = NewInteractable;
	CurrentTargetActor = NewActor;

	// Notify listeners (BP/C++)
	OnInteractTargetChanged.Broadcast(CurrentTargetActor.Get());

	if (CurrentInteractable.IsValid())
	{
		ShowPromptFor(CurrentInteractable.Get());
	}
	else
	{
		HidePrompt();
	}
}

void URpg_InteractionComponent::ShowPromptFor(UObject* InteractableObj)
{
	if (!PromptWidget || !InteractableObj) return;

	const FInteractDisplayData Data = IInteractable::Execute_GetDisplayData(InteractableObj);

	// Icon ggf. synchron laden (einfach und ausreichend für kleine Icons)
	UTexture2D* Icon = Data.Icon.IsNull() ? nullptr : Data.Icon.LoadSynchronous();

	FInteractDisplayData Resolved = Data;
	Resolved.Icon = Icon; // ersetzt SoftRef durch geladenes Icon

	PromptWidget->SetPromptData(Resolved);
	PromptWidget->SetPromptVisible(true);
}

void URpg_InteractionComponent::HidePrompt()
{
	if (PromptWidget) PromptWidget->SetPromptVisible(false);
}

void URpg_InteractionComponent::TryInteract()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		if (APlayerController* OwnerPC = Cast<APlayerController>(GetOwner()))
		{
			OwnerPawn = OwnerPC->GetPawn();
		}
	}
	if (!CurrentInteractable.IsValid() || !OwnerPawn) return;

	// Clientseitig sanity-check
	if (!IInteractable::Execute_CanInteract(CurrentInteractable.Get(), OwnerPawn)) return;

	// Server anweisen (autoritativer Call)
	Server_Interact(CurrentTargetActor.Get());
}

void URpg_InteractionComponent::Server_Interact_Implementation(AActor* TargetActor)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		if (APlayerController* OwnerPC = Cast<APlayerController>(GetOwner()))
		{
			OwnerPawn = OwnerPC->GetPawn();
		}
	}
	if (!TargetActor) return;

	UObject* InteractableObj = FindInteractableOn(TargetActor, nullptr);
	if (!InteractableObj || !OwnerPawn) return;

	// Sicherheit: Reichweite/CanInteract erneut am Server prüfen
	if (!IInteractable::Execute_CanInteract(InteractableObj, OwnerPawn)) return;

	IInteractable::Execute_Interact(InteractableObj, OwnerPawn);
}




