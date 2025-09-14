// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Interact/Rpg_InteractionComponent.h"

#include "InventoryManagement/Interact/Rpg_InteractableComponent.h"
#include "InventoryManagement/Interact/Interface/Interactable.h"
#include "InventoryManagement/Interact/Widget/InteractPromptWidget.h"


void URpg_InteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	PrimaryComponentTick.bCanEverTick = true;

	if (PromptWidgetClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetController() : nullptr))
		{
			PromptWidget = CreateWidget<UInteractPromptWidget>(PC, PromptWidgetClass);
			if (PromptWidget) { PromptWidget->AddToViewport(PromptZOrder); PromptWidget->SetPromptVisible(false); }
		}
	}
}

void URpg_InteractionComponent::TickComponent(float, ELevelTick, FActorComponentTickFunction*)
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
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!OwnerPawn || !PC) return;

	FVector ViewLoc; FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector Start = ViewLoc;
	const FVector End   = Start + ViewRot.Vector() * TraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), /*bTraceComplex*/false, OwnerPawn);
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

	if (CurrentInteractable.IsValid()) ShowPromptFor(CurrentInteractable.Get());
	else                              HidePrompt();
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
	if (!OwnerPawn || !CurrentInteractable.IsValid()) return;

	// Clientseitig sanity-check
	if (!IInteractable::Execute_CanInteract(CurrentInteractable.Get(), OwnerPawn)) return;

	// Server anweisen (autoritativer Call)
	Server_Interact(CurrentTargetActor.Get());
}

void URpg_InteractionComponent::Server_Interact_Implementation(AActor* TargetActor)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !TargetActor) return;

	UObject* InteractableObj = FindInteractableOn(TargetActor, nullptr);
	if (!InteractableObj) return;

	// Sicherheit: Reichweite/CanInteract erneut am Server prüfen
	if (!IInteractable::Execute_CanInteract(InteractableObj, OwnerPawn)) return;

	IInteractable::Execute_Interact(InteractableObj, OwnerPawn);
}




