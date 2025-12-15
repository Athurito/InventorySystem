// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractorComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "InteractableComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "SimpleInteractionSystem/Data/InteractionDefinition.h"

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;

	// Fokus/UI ist lokal, Execute läuft über GAS (Server-authoritativ)
	SetIsReplicatedByDefault(false);
}

void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UInteractorComponent::IsLocalInteractor() const
{
	// Nur der owning client soll Targets suchen und UI togglen
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return true; // falls es mal an keinem Pawn hängt: "lokal" akzeptieren

	return Pawn->IsLocallyControlled();
}

APlayerController* UInteractorComponent::GetLocalPlayerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return nullptr;

	return Cast<APlayerController>(Pawn->GetController());
}

static void GetInteractorView(const AActor* OwnerActor, FVector& OutLoc, FRotator& OutRot)
{
	OutLoc = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
	OutRot = OwnerActor ? OwnerActor->GetActorRotation() : FRotator::ZeroRotator;

	const APawn* Pawn = Cast<APawn>(OwnerActor);
	if (Pawn)
	{
		if (AController* C = Pawn->GetController())
		{
			C->GetPlayerViewPoint(OutLoc, OutRot);
			return;
		}

		Pawn->GetActorEyesViewPoint(OutLoc, OutRot);
		return;
	}
}

void UInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
	Super::TickComponent(DeltaTime, TickType, ThisTick);

	if (!IsLocalInteractor())
	{
		return;
	}

	UInteractableComponent* NewTarget = FindBestInteractable();

	if (NewTarget != Current)
	{
		HandleFocusChanged(Current, NewTarget);
		Current = NewTarget;
	}
}

void UInteractorComponent::RefreshFocus()
{
	if (!IsLocalInteractor())
	{
		return;
	}

	UInteractableComponent* NewTarget = FindBestInteractable();
	if (NewTarget != Current)
	{
		HandleFocusChanged(Current, NewTarget);
		Current = NewTarget;
	}
}

void UInteractorComponent::HandleFocusChanged(UInteractableComponent* OldTarget, UInteractableComponent* NewTarget)
{
	APlayerController* PC = GetLocalPlayerController();

	if (OldTarget)
	{
		OldTarget->SetLocalFocused(false, PC);
	}

	if (NewTarget)
	{
		NewTarget->SetLocalFocused(true, PC);
	}
}

UInteractableComponent* UInteractorComponent::FindBestInteractable() const
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return nullptr;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	GetInteractorView(OwnerActor, ViewLoc, ViewRot);

	const FVector Start = ViewLoc;
	const FVector End = Start + (ViewRot.Vector() * TraceDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), false);
	Params.AddIgnoredActor(OwnerActor);

	TArray<FHitResult> Hits;
	bool bHit = false;

	if (bUseSphereTrace)
	{
		bHit = World->SweepMultiByChannel(
			Hits,
			Start,
			End,
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(TraceRadius),
			Params
		);
	}
	else
	{
		FHitResult SingleHit;
		bHit = World->LineTraceSingleByChannel(SingleHit, Start, End, ECC_Visibility, Params);
		if (bHit)
		{
			Hits.Add(SingleHit);
		}
	}

	if (bDebugDraw)
	{
		if (bUseSphereTrace)
		{
			DrawDebugLine(World, Start, End, FColor::Cyan, false, 0.f, 0, 1.f);
			DrawDebugSphere(World, End, TraceRadius, 12, FColor::Cyan, false, 0.f, 0, 1.f);
		}
		else
		{
			DrawDebugLine(World, Start, End, FColor::Cyan, false, 0.f, 0, 1.f);
		}
	}

	if (!bHit)
	{
		return nullptr;
	}

	// Scoring: wähle bestes Interactable aus allen Hits
	UInteractableComponent* Best = nullptr;
	float BestScore = -FLT_MAX;

	const FVector ViewDir = ViewRot.Vector();

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) continue;

		UInteractableComponent* Interactable = HitActor->FindComponentByClass<UInteractableComponent>();
		if (!Interactable) continue;

		if (!Interactable->bEnabled || !Interactable->Definition) continue;

		// Soft-check für UI (harte Validierung kommt serverseitig in Ability)
		if (!Interactable->CanInteract(OwnerActor)) continue;

		// Distanz check gegen Definition
		const float MaxDist = Interactable->Definition->MaxDistance;
		const float DistSq = FVector::DistSquared(OwnerActor->GetActorLocation(), HitActor->GetActorLocation());
		if (DistSq > FMath::Square(MaxDist)) continue;

		// Score = "wie mittig im Blick" + "wie nah"
		const FVector ToTarget = (Hit.ImpactPoint - Start);
		const float Dist = FMath::Max(ToTarget.Length(), 1.f);
		const FVector ToTargetDir = ToTarget / Dist;

		const float Dot = FVector::DotProduct(ViewDir, ToTargetDir); // [-1..1], höher = mehr center
		const float DistanceScore = 1.f / Dist;                      // höher = näher

		const float Score = (Dot * ViewDotWeight) + (DistanceScore * DistanceWeight);

		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Interactable;
		}

		if (bDebugDraw)
		{
			DrawDebugPoint(World, Hit.ImpactPoint, 10.f, FColor::Green, false, 0.f);
		}
	}

	return Best;
}

UAbilitySystemComponent* UInteractorComponent::GetASC() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	// Standard: ASC am OwnerActor (Character/Pawn)
	// Lyra/PlayerState-Setup: hier müsstest du den ASC vom PlayerState holen.
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
}

void UInteractorComponent::TryInteract()
{
	if (!IsLocalInteractor())
	{
		return;
	}

	UInteractableComponent* Target = Current;
	if (!Target || !Target->Definition)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// Minimal clientseitige Vorchecks (serverseitig in Ability nochmal!)
	if (!Target->bEnabled || !Target->CanInteract(OwnerActor))
	{
		return;
	}

	const float MaxDist = Target->Definition->MaxDistance;
	const float DistSq = FVector::DistSquared(OwnerActor->GetActorLocation(), Target->GetOwner()->GetActorLocation());
	if (DistSq > FMath::Square(MaxDist))
	{
		return;
	}

	SendGameplayEventToASC(Target);
}

void UInteractorComponent::SendGameplayEventToASC(UInteractableComponent* Target) const
{
	if (!Target || !Target->Definition)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	AActor* TargetActor = Target->GetOwner();
	if (!OwnerActor || !TargetActor)
	{
		return;
	}

	const FGameplayTag EventTag = Target->Definition->GameplayEventTag;
	if (!EventTag.IsValid())
	{
		// Du willst absichtlich data-driven: wenn das nicht gesetzt ist, passiert nix.
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = OwnerActor;
	EventData.Target = TargetActor;

	// Data-driven payload:
	EventData.OptionalObject  = Target->Definition; // welches Interaction-DataAsset
	EventData.OptionalObject2 = Target;             // optional: Component reference
	EventData.InstigatorTags.AddTag(Target->Definition->InteractionTag); // z.B. Interaction.OpenDoor

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, EventData);
}

