// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractionManagement/Data/InteractableDataAsset.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class APawn;

UINTERFACE(BlueprintType)
class RPGINVENTORY_API UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Runtime interaction interface with Blueprint support
 */
class RPGINVENTORY_API IInteractable
{
	GENERATED_BODY()

public:
	// Display data used by UI prompt
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interaction")
	FInteractDisplayData GetDisplayData() const;

	// Is interaction currently allowed?
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interaction")
	bool CanInteract(APawn* Instigator) const;

	// Perform the interaction (should be executed on the server)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interaction")
	void Interact(APawn* Instigator);
};
