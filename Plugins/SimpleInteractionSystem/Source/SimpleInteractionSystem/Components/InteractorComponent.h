// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractorComponent.generated.h"


class UAbilitySystemComponent;
class UInteractableComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SIMPLEINTERACTIONSYSTEM_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractorComponent();

	// Trace settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Trace")
	float TraceDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Trace")
	float TraceRadius = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Trace")
	bool bUseSphereTrace = true;

	// Selection settings (wenn mehrere Treffer)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Selection")
	float ViewDotWeight = 2.0f; // höher = stärker "in der Mitte des Bilds"

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Selection")
	float DistanceWeight = 1.0f; // höher = stärker "näher gewinnt"

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Debug")
	bool bDebugDraw = false;

	// Current target (local)
	UFUNCTION(BlueprintCallable, Category="Interaction")
	UInteractableComponent* GetCurrentInteractable() const { return Current; }

	// Call from Input (CommonUI / Enhanced Input -> BP -> this)
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void TryInteract();

	// Optional: wenn du Focus extern refreshen willst
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void RefreshFocus();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UInteractableComponent> Current = nullptr;

	// Internals
	bool IsLocalInteractor() const;
	APlayerController* GetLocalPlayerController() const;

	UInteractableComponent* FindBestInteractable() const;
	void HandleFocusChanged(UInteractableComponent* OldTarget, UInteractableComponent* NewTarget);

	UAbilitySystemComponent* GetASC() const;
	void SendGameplayEventToASC(UInteractableComponent* Target) const;
};
