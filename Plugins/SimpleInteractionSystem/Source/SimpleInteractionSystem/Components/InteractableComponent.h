// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"


class UArrowComponent;
class UWidgetComponent;
class UUserWidget;
struct FGameplayTag;
class UInteractionDefinition;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SIMPLEINTERACTIONSYSTEM_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();

	// --------- Data ---------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<UInteractionDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Interaction")
	bool bEnabled = true;

	UFUNCTION(BlueprintCallable, Category="Interaction")
	UInteractionDefinition* GetDefinition() const { return Definition; }

	// --------- Interaction gating ---------

	/** Soft-check for UI + client. Hard-check should be repeated server-side in the Router Ability. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const;

	/** Called by server-side ability after validation (recommended). Can be overridden in BP. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void OnInteractionExecuted(AActor* Interactor, FGameplayTag InteractionTag);
	virtual void OnInteractionExecuted_Implementation(AActor* Interactor, FGameplayTag InteractionTag);

	// --------- UI (Actor-attached) ---------

	// Name of ArrowComponent used as anchor. If not found, fallback arrow is created.
	UPROPERTY(EditDefaultsOnly, Category="Interaction|UI")
	FName AnchorArrowName = TEXT("InteractionAnchor");

	// Widget class to instantiate on the interactable (CommonUI widget is fine).
	UPROPERTY(EditDefaultsOnly, Category="Interaction|UI")
	TSubclassOf<UUserWidget> WidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Interaction|UI")
	FVector2D DrawSize = FVector2D(260.f, 64.f);

	UPROPERTY(EditDefaultsOnly, Category="Interaction|UI")
	bool bBillboardToLocalCamera = true;

	/** Called by InteractorComponent locally to show/hide widget for the local player */
	UFUNCTION(BlueprintCallable, Category="Interaction|UI")
	void SetLocalFocused(bool bFocused, APlayerController* LocalPC);

	UFUNCTION(BlueprintCallable, Category="Interaction|UI")
	UWidgetComponent* GetWidgetComponent() const { return WidgetComponent; }

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;

private:
	UArrowComponent* FindOrCreateAnchorArrow();
	void EnsureWidgetCreated();
	void BindWidgetIfPossible();

	UPROPERTY(Transient)
	TObjectPtr<UArrowComponent> AnchorArrow = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> WidgetComponent = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> FocusedLocalPC;

	bool bIsLocallyFocused = false;
	
};
