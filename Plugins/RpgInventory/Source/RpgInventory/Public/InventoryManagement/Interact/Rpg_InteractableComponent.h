// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/Interactable.h"
#include "Rpg_InteractableComponent.generated.h"

class APawn;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteraction, AActor*, NewTarget);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API URpg_InteractableComponent : public UActorComponent, public IInteractable
{
	GENERATED_BODY()

public:
	URpg_InteractableComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	UInteractableDataAsset* InteractableData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	FInteractDisplayData InlineDisplayData;

	// Distance Check (optional, fürs CanInteract)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	float MaxUseDistance = 250.f;
	
	// IInteractable implementations (BlueprintNativeEvent)
	virtual FInteractDisplayData GetDisplayData_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Instigator) const override;
	virtual void Interact_Implementation(APawn* Instigator) override;


private:
	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FOnInteraction OnInteraction;
};
