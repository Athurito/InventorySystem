#pragma once

#include "CoreMinimal.h"
#include "InventorySystemPlayerController.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "InventoryGASTestController.generated.h"

class UInventoryTestAttributeSet;

UCLASS()
class INVENTORYSYSTEM_API AInventoryGASTestController : public AInventorySystemPlayerController, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AInventoryGASTestController();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	// Ability System Component used for testing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// Simple AttributeSet for testing inventory-related attributes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UInventoryTestAttributeSet> AttributeSet;
};
