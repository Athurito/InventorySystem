// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryManagement/Interact/Data/InteractableDataAsset.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class RPGINVENTORY_API IInteractable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual FInteractDisplayData GetDisplayData() const = 0;
	// Darf man gerade interagieren? (z.B. gesperrt, zu weit weg, etc.)
	virtual bool CanInteract(AActor* Instigator) const { return true; }

	// Die eigentliche Interaktion (Server-seitig ausführen!)
	virtual void Interact(AActor* Instigator) = 0;
};
