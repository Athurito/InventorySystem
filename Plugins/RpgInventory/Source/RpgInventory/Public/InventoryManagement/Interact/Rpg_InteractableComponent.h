// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/Interactable.h"
#include "Rpg_InteractableComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API URpg_InteractableComponent : public UActorComponent, public IInteractable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	UInteractableDataAsset* DataAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	FInteractDisplayData InlineDisplayData;

	// Distance Check (optional, fürs CanInteract)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	float MaxUseDistance = 250.f;
	
	virtual FInteractDisplayData GetDisplayData() const override;
	
	virtual bool CanInteract(AActor* Instigator) const override;

	// Standard-Interact: überschreibbar in BP oder per Subclass
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category="Interaction")
	void BP_OnInteract(AActor* Instigator);

	virtual void Interact(AActor* Instigator) override
	{
		// Default: nur BP Event triggern
		BP_OnInteract(Instigator);
	}
};
