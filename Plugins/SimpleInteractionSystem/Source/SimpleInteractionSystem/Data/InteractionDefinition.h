// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "InteractionDefinition.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FInteractionEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Interactor = nullptr;   // Pawn/Character
	UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Interactable = nullptr; // Target Actor
	UPROPERTY(BlueprintReadOnly) FGameplayTag InteractionTag;              // z.B. Interaction.Open
	UPROPERTY(BlueprintReadOnly) FGameplayTagContainer ContextTags;        // optional
};

UCLASS()
class SIMPLEINTERACTIONSYSTEM_API UInteractionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FText Description;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FGameplayTag InteractionTag;   // Interaction.OpenChest etc

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float MaxDistance = 250.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bHoldToInteract = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(EditCondition="bHoldToInteract"))
	float HoldTime = 0.35f;

	// Server-side Gatekeeping (optional)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FGameplayTagContainer RequiredInteractorTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FGameplayTagContainer BlockedInteractorTags;

	// Der “Ein-Klick” Trigger für GAS GameplayEvent
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag GameplayEventTag; // z.B. Event.Interaction.Execute

	// UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UTexture2D> Icon = nullptr;
};
