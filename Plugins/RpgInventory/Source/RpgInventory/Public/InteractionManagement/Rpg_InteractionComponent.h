// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/InteractableDataAsset.h"
#include "Interface/Interactable.h"
#include "Rpg_InteractionComponent.generated.h"

class UInteractPromptWidget;
class UInputAction;
class APlayerController;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractTargetChanged, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractPromptChanged, bool, bVisible, FInteractDisplayData, DisplayData);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), BlueprintType)
class RPGINVENTORY_API URpg_InteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URpg_InteractionComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Trace") float TraceDistance = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Trace") TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Trace") float UpdateInterval = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Input") UInputAction* InteractInputAction = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Debug") bool bDebug = false;
	
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void TryInteract();
	
	UFUNCTION(BlueprintCallable, Category="Interaction")
	bool HasInteractable() const { return CurrentInteractable.GetObject() != nullptr; }

	UFUNCTION(BlueprintCallable, Category="Interaction")
	AActor* GetCurrentTargetActor() const { return CurrentTargetActor.Get(); }
	
	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FOnInteractTargetChanged OnInteractTargetChanged;

	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FOnInteractPromptChanged OnInteractPromptChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	
	TWeakObjectPtr<APlayerController> OwnerPC;
	TWeakObjectPtr<APawn> CachedPawn;

	// Aktuelle Auswahl
	TWeakObjectPtr<AActor> CurrentTargetActor;
	TScriptInterface<IInteractable> CurrentInteractable; // Actor ODER Component mit Interface

	// Intern
	void UpdateTrace();
	TScriptInterface<IInteractable> FindInteractableOn(AActor* Actor, UPrimitiveComponent* HitComp) const;
	void OnTargetChanged(const TScriptInterface<IInteractable>& NewInteractable, AActor* NewActor);

	UFUNCTION()
	void OnObservedActorDestroyed(AActor* DestroyedActor);

	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	// Server-RPC
	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* TargetActor);
};
