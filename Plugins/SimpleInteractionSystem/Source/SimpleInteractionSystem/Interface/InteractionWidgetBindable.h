// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionWidgetBindable.generated.h"

class UInteractableComponent;
// This class does not need to be modified.
UINTERFACE()
class UInteractionWidgetBindable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SIMPLEINTERACTIONSYSTEM_API IInteractionWidgetBindable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction|UI")
	void BindInteractable(UInteractableComponent* Interactable);
};
