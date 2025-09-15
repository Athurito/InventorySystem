// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Rpg_InteractionComponent.generated.h"

class UInteractPromptWidget;
class UInputAction;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractTargetChanged, AActor*, NewTarget);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), BlueprintType)
class RPGINVENTORY_API URpg_InteractionComponent : public UActorComponent
{
	GENERATED_BODY()


public:
	// Trace settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trace") float TraceDistance = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trace") TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	// Trace throttling: 0 = every tick, >0 = seconds between traces
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trace") float UpdateInterval = 0.f;

	// UI settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI") TSubclassOf<UInteractPromptWidget> PromptWidgetClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI") int32 PromptZOrder = 10;

	// Input (Enhanced Input optional)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") UInputAction* InteractInputAction = nullptr;

	// Debug
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Debug") bool bDebug = false;

	// Input ruft das hier auf (Client)
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void TryInteract();

	// Für Blueprints nützlich
	UFUNCTION(BlueprintCallable, Category="Interaction")
	bool HasInteractable() const { return CurrentInteractable.IsValid(); }
	UFUNCTION(BlueprintCallable, Category="Interaction")
	AActor* GetCurrentTargetActor() const { return CurrentTargetActor.Get(); }

	// Notify when target changes
	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FOnInteractTargetChanged OnInteractTargetChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	TWeakObjectPtr<AActor>  CurrentTargetActor;
	TWeakObjectPtr<UObject> CurrentInteractable; // Actor ODER Component, die UInteractable implementiert
	UPROPERTY() UInteractPromptWidget* PromptWidget = nullptr;

	float TimeSinceUpdate = 0.f;

	void UpdateTrace();
	UObject* FindInteractableOn(AActor* Actor, UPrimitiveComponent* HitComp) const;
	void OnTargetChanged(UObject* NewInteractable, AActor* NewActor);

	void ShowPromptFor(UObject* InteractableObj);
	void HidePrompt();

	// Server-RPC (führt Interact autoritativ aus)
	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* TargetActor);
};
