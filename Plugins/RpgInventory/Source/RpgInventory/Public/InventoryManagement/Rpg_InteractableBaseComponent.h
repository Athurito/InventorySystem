// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionManagement/Interface/Interactable.h"
#include "Rpg_InteractableBaseComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API URpg_InteractableBaseComponent : public UActorComponent, public IInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URpg_InteractableBaseComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	bool bEnabled = true;

	// Distance Check (optional, fürs CanInteract)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	float MaxUseDistance = 250.f;
};
