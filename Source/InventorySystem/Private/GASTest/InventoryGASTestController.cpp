#include "../../Public/GASTest/InventoryGASTestController.h"
#include "../../Public/GASTest/InventoryTestAttributeSet.h"

AInventoryGASTestController::AInventoryGASTestController()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UInventoryTestAttributeSet>(TEXT("InventoryTestAttributeSet"));
}

UAbilitySystemComponent* AInventoryGASTestController::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
