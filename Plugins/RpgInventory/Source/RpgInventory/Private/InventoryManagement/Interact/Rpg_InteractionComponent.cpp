// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Interact/Rpg_InteractionComponent.h"

#include "InventoryManagement/Interact/Interface/Interactable.h"
#include "InventoryManagement/Interact/Widget/Rpg_HUDWidget.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"


URpg_InteractionComponent::URpg_InteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bAutoActivate = true;
	SetComponentTickInterval(UpdateInterval);
}

void URpg_InteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// OwnerPC bestimmen
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPC = Cast<APlayerController>(OwnerPawn->GetController());
	}
	else
	{
		OwnerPC = Cast<APlayerController>(GetOwner());
	}

	if (!OwnerPC.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("InteractionComponent: Kein PlayerController gefunden -> disabled."));
		SetComponentTickEnabled(false);
		return;
	}

	CachedPawn = OwnerPC->GetPawn();

	// Optional automatisches Input-Binding (wenn Enhanced Input vorhanden)
	if (InteractInputAction && OwnerPC->IsLocalController())
	{
		UEnhancedInputComponent* EIC = nullptr;
		if (CachedPawn.IsValid()) EIC = Cast<UEnhancedInputComponent>(CachedPawn->InputComponent);
		if (!EIC) EIC = Cast<UEnhancedInputComponent>(OwnerPC->InputComponent);

		if (EIC)
		{
			EIC->BindAction(InteractInputAction, ETriggerEvent::Started, this, &URpg_InteractionComponent::TryInteract);
		}
	}
}

void URpg_InteractionComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	// Pawn-Wechsel erkennen
	if (OwnerPC.IsValid())
	{
		APawn* NewPawn = OwnerPC->GetPawn();
		if (NewPawn != CachedPawn.Get())
		{
			HandlePossessedPawnChanged(CachedPawn.Get(), NewPawn);
		}
	}
	UpdateTrace();
}

TScriptInterface<IInteractable> URpg_InteractionComponent::FindInteractableOn(AActor* Actor, UPrimitiveComponent* HitComp) const
{
	TScriptInterface<IInteractable> Result;
	if (!Actor) return Result;

	auto TryMake = [&](UObject* Obj)
	{
		if (!Obj) return;
		if (Obj->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
		{
			Result.SetObject(Obj);
			Result.SetInterface(Cast<IInteractable>(Obj));
		}
	};

	// 1) Actor selbst?
	TryMake(Actor);

	// 2) spezielles Hit-Component?
	if (!Result && HitComp) TryMake(HitComp);

	// 3) Interactable-Component?
	if (!Result)
	{
		TArray<UActorComponent*> Comps;
		Actor->GetComponents(Comps);
		for (UActorComponent* C : Comps)
		{
			if (!C) continue;
			if (C->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
			{
				TryMake(C);
				break;
			}
		}
	}

	return Result;
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
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), false, IgnoreActor);
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

	if (bDebug)
	{
		DrawDebugLine(GetWorld(), Start, Hit.bBlockingHit ? Hit.ImpactPoint : End,
		              Hit.bBlockingHit ? FColor::Green : FColor::Red, false, 0.f, 0, 0.5f);
	}

	TScriptInterface<IInteractable> NewInteractable;
	AActor* NewActor = nullptr;

	if (Hit.bBlockingHit && Hit.GetActor())
	{
		NewInteractable = FindInteractableOn(Hit.GetActor(), Hit.GetComponent());
		NewActor = Hit.GetActor();

		// Früh prüfen, ob Interact erlaubt ist -> sonst kein Prompt
		if (!CachedPawn.IsValid() ||
		    (NewInteractable && !NewInteractable->Execute_CanInteract(NewInteractable.GetObject(), CachedPawn.Get())))
		{
			NewInteractable = TScriptInterface<IInteractable>();
			NewActor = nullptr;
		}
	}

	// Änderung?
	if (CurrentInteractable.GetObject() != NewInteractable.GetObject())
	{
		OnTargetChanged(NewInteractable, NewActor);
	}
	else if (!NewInteractable && CurrentInteractable.GetObject())
	{
		// Sicherheitsnetz
		OnTargetChanged(TScriptInterface<IInteractable>(), nullptr);
	}
}

void URpg_InteractionComponent::OnTargetChanged(const TScriptInterface<IInteractable>& NewInteractable, AActor* NewActor)
{
	// Unbind altes Actor-Destruction-Event
	if (AActor* PrevActor = CurrentTargetActor.Get())
	{
		PrevActor->OnDestroyed.RemoveDynamic(this, &URpg_InteractionComponent::OnObservedActorDestroyed);
	}

	CurrentInteractable = NewInteractable;
	CurrentTargetActor = NewActor;

	// Neues Actor-Destruction-Event
	if (AActor* BoundActor = CurrentTargetActor.Get())
	{
		BoundActor->OnDestroyed.AddUniqueDynamic(this, &URpg_InteractionComponent::OnObservedActorDestroyed);
	}

	// 1) reines Gameplay-Event (z.B. für Sounds etc.)
	OnInteractTargetChanged.Broadcast(CurrentTargetActor.Get());

	// 2) UI-Event mit Daten
	if (CurrentInteractable.GetObject())
	{
		const FInteractDisplayData Data = CurrentInteractable->Execute_GetDisplayData(CurrentInteractable.GetObject());
		OnInteractPromptChanged.Broadcast(true, Data);
	}
	else
	{
		OnInteractPromptChanged.Broadcast(false, FInteractDisplayData{});
	}
}

void URpg_InteractionComponent::OnObservedActorDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor && DestroyedActor == CurrentTargetActor.Get())
	{
		CurrentTargetActor = nullptr;
		CurrentInteractable = TScriptInterface<IInteractable>();
		OnInteractTargetChanged.Broadcast(nullptr);
		OnInteractPromptChanged.Broadcast(false, FInteractDisplayData{});
	}
}

void URpg_InteractionComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	CachedPawn = NewPawn;
	if (!CachedPawn.IsValid() && (CurrentInteractable.GetObject() || CurrentTargetActor.IsValid()))
	{
		CurrentInteractable = TScriptInterface<IInteractable>();
		CurrentTargetActor = nullptr;
		OnInteractTargetChanged.Broadcast(nullptr);
		OnInteractPromptChanged.Broadcast(false, FInteractDisplayData{});
	}
}

void URpg_InteractionComponent::TryInteract()
{
	if (!CurrentInteractable.GetObject() || !CachedPawn.IsValid()) return;
	if (!CurrentInteractable->Execute_CanInteract(CurrentInteractable.GetObject(), CachedPawn.Get())) return;
	Server_Interact(CurrentTargetActor.Get());
}

void URpg_InteractionComponent::Server_Interact_Implementation(AActor* TargetActor)
{
	if (!TargetActor) return;

	// Autoritativer Pawn auf dem Server
	APawn* ServerPawn = nullptr;
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		ServerPawn = PC->GetPawn();
	}
	else if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		ServerPawn = OwnerPawn;
	}
	if (!ServerPawn) return;

	TScriptInterface<IInteractable> InteractableObj = FindInteractableOn(TargetActor, nullptr);
	if (!InteractableObj.GetObject()) return;

	if (!InteractableObj->Execute_CanInteract(InteractableObj.GetObject(), ServerPawn)) return;

	InteractableObj->Execute_Interact(InteractableObj.GetObject(), ServerPawn);
}




