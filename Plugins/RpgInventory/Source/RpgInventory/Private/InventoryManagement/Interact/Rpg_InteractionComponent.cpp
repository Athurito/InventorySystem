// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Interact/Rpg_InteractionComponent.h"

#include "InventoryManagement/Interact/Rpg_InteractableComponent.h"
#include "InventoryManagement/Interact/Interface/Interactable.h"
#include "InventoryManagement/Interact/Widget/InteractPromptWidget.h"
#include "InventoryManagement/Interact/Widget/Rpg_HUDWidget.h"
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

	// Resolve and cache controller/pawn
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	OwnerPC = nullptr;
	if (OwnerPawn)
	{
		OwnerPC = Cast<APlayerController>(OwnerPawn->GetController());
	}
	else
	{
		OwnerPC = Cast<APlayerController>(GetOwner());
	}
	if (!OwnerPC.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("URpg_InteractionComponent: Owner is neither a PlayerController nor has one; interaction will be disabled."));
		SetComponentTickEnabled(false);
		return;
	}
	CachedPawn = OwnerPC->GetPawn();

	// Note: Not binding to engine delegates here to keep things generic across owners.
	// We'll detect pawn changes in Tick and update cache accordingly.

	// Create always-on HUD if class provided (local controller only). If present, it will host the prompt.
	if (HUDWidgetClass && OwnerPC->IsLocalController())
	{
		HUDWidget = CreateWidget<URpg_HUDWidget>(OwnerPC.Get(), HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(HUDZOrder);
			// Ensure prompt is hidden at start (BP default might be Visible)
			HUDWidget->HideInteractPrompt();
			// Crosshair visibility can be managed in BP or here if needed.
		}
	}

	// Legacy: create standalone prompt only if no HUD is present
	if (!HUDWidget && PromptWidgetClass && OwnerPC->IsLocalController())
	{
		PromptWidget = CreateWidget<UInteractPromptWidget>(OwnerPC.Get(), PromptWidgetClass);
		if (PromptWidget)
		{
			PromptWidget->AddToViewport(PromptZOrder);
			PromptWidget->SetPromptVisible(false);
		}
	}

	// Try to auto-bind Enhanced Input action if configured
	if (InteractInputAction && OwnerPC->IsLocalController())
	{
		UEnhancedInputComponent* EIC = nullptr;
		if (CachedPawn.IsValid() && CachedPawn->InputComponent)
		{
			EIC = Cast<UEnhancedInputComponent>(CachedPawn->InputComponent);
		}
		if (!EIC && OwnerPC->InputComponent)
		{
			EIC = Cast<UEnhancedInputComponent>(OwnerPC->InputComponent);
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
	else if (!InteractInputAction)
	{
		UE_LOG(LogTemp, Verbose, TEXT("URpg_InteractionComponent: InteractInputAction is null; assign it in the component settings."));
	}
}

void URpg_InteractionComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	// Detect pawn change
	if (OwnerPC.IsValid())
	{
		APawn* NewPawn = OwnerPC->GetPawn();
		if (NewPawn != CachedPawn.Get())
		{
			HandlePossessedPawnChanged(CachedPawn.Get(), NewPawn);
		}
	}

	// Throttle traces if requested
	if (UpdateInterval > 0.f)
	{
		TimeSinceLastTrace += DeltaTime;
		if (TimeSinceLastTrace < UpdateInterval)
		{
			return;
		}
		TimeSinceLastTrace = 0.f;
	}
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
	if (!OwnerPC.IsValid()) return;

	FVector ViewLoc; FRotator ViewRot;
	OwnerPC->GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector Start = ViewLoc;
	const FVector End   = Start + ViewRot.Vector() * TraceDistance;

	FHitResult Hit;
	AActor* IgnoreActor = CachedPawn.IsValid() ? static_cast<AActor*>(CachedPawn.Get()) : Cast<AActor>(OwnerPC.Get());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), /*bTraceComplex*/false, IgnoreActor);
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

	if (bDebug) DrawDebugLine(GetWorld(), Start, Hit.bBlockingHit ? Hit.ImpactPoint : End, Hit.bBlockingHit ? FColor::Green : FColor::Red, false, 0.f, 0, 0.5f);

	UObject* NewInteractable = nullptr;
	AActor*  NewActor = nullptr;

	if (Hit.bBlockingHit && Hit.GetActor())
	{
		NewInteractable = FindInteractableOn(Hit.GetActor(), Hit.GetComponent());
		NewActor = Hit.GetActor();
		// Optional: check CanInteract early; if no pawn, don't show prompt
		if (!CachedPawn.IsValid() || (NewInteractable && !IInteractable::Execute_CanInteract(NewInteractable, CachedPawn.Get())))
		{
			NewInteractable = nullptr;
			NewActor = nullptr;
		}
	}

	if (CurrentInteractable.Get() != NewInteractable)
	{
		OnTargetChanged(NewInteractable, NewActor);
	}
	else if (!NewInteractable && CurrentInteractable.IsValid())
	{
		// Defensive: ensure prompt hidden if we lost pawn mid-frame
		HidePrompt();
	}
}

void URpg_InteractionComponent::OnTargetChanged(UObject* NewInteractable, AActor* NewActor)
{
	// Unbind from previous actor destruction
	if (AActor* PrevActor = CurrentTargetActor.Get())
	{
		PrevActor->OnDestroyed.RemoveDynamic(this, &URpg_InteractionComponent::OnObservedActorDestroyed);
	}

	CurrentInteractable = NewInteractable;
	CurrentTargetActor = NewActor;

	// Bind to new actor destruction to auto-hide prompt when it goes away
	if (AActor* BoundActor = CurrentTargetActor.Get())
	{
		BoundActor->OnDestroyed.AddUniqueDynamic(this, &URpg_InteractionComponent::OnObservedActorDestroyed);
	}

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
	if (!InteractableObj) return;

	const FInteractDisplayData Data = IInteractable::Execute_GetDisplayData(InteractableObj);

	// Icon ggf. synchron laden (einfach und ausreichend für kleine Icons)
	UTexture2D* Icon = Data.Icon.IsNull() ? nullptr : Data.Icon.LoadSynchronous();

	FInteractDisplayData Resolved = Data;
	Resolved.Icon = Icon; // ersetzt SoftRef durch geladenes Icon

	if (HUDWidget)
	{
		HUDWidget->ShowInteractPrompt(Resolved);
	}
	else if (PromptWidget)
	{
		PromptWidget->SetPromptData(Resolved);
		PromptWidget->SetPromptVisible(true);
	}
}

void URpg_InteractionComponent::HidePrompt() const
{
	if (HUDWidget)
	{
		HUDWidget->HideInteractPrompt();
	}
	else if (PromptWidget)
	{
		PromptWidget->SetPromptVisible(false);
	}
}

void URpg_InteractionComponent::OnObservedActorDestroyed(AActor* DestroyedActor)
{
	// If the destroyed actor was our current target, clear and hide
	if (DestroyedActor && DestroyedActor == CurrentTargetActor.Get())
	{
		CurrentTargetActor = nullptr;
		CurrentInteractable = nullptr;
		HidePrompt();
		OnInteractTargetChanged.Broadcast(nullptr);
	}
}

void URpg_InteractionComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	CachedPawn = NewPawn;
	// Hide/clear when we lose pawn
	if (!CachedPawn.IsValid())
	{
		if (CurrentInteractable.IsValid() || CurrentTargetActor.IsValid())
		{
			CurrentInteractable = nullptr;
			CurrentTargetActor = nullptr;
			HidePrompt();
			OnInteractTargetChanged.Broadcast(nullptr);
		}
	}
}

void URpg_InteractionComponent::TryInteract()
{
	if (!CurrentInteractable.IsValid() || !CachedPawn.IsValid()) return;

	// Clientseitig sanity-check
	if (!IInteractable::Execute_CanInteract(CurrentInteractable.Get(), CachedPawn.Get())) return;

	// Server anweisen (autoritativer Call)
	Server_Interact(CurrentTargetActor.Get());
}

void URpg_InteractionComponent::Server_Interact_Implementation(AActor* TargetActor)
{
	if (!TargetActor) return;

	// Resolve authoritative pawn on server
	APawn* ServerPawn = nullptr;
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		ServerPawn = PC->GetPawn();
	}
	else if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		// Fallback for non-controller owners
		ServerPawn = OwnerPawn;
	}
	if (!ServerPawn) return;

	UObject* InteractableObj = FindInteractableOn(TargetActor, nullptr);
	if (!InteractableObj) return;

	if (!IInteractable::Execute_CanInteract(InteractableObj, ServerPawn)) return;

	IInteractable::Execute_Interact(InteractableObj, ServerPawn);
}




