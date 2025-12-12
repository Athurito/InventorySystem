#include "../../Public/GASTest/InventoryTestAttributeSet.h"
#include "Net/UnrealNetwork.h"

void UInventoryTestAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryTestAttributeSet, CarryCapacity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UInventoryTestAttributeSet, CurrentWeight, COND_None, REPNOTIFY_Always);
}

void UInventoryTestAttributeSet::OnRep_CarryCapacity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryTestAttributeSet, CarryCapacity, OldValue);
}

void UInventoryTestAttributeSet::OnRep_CurrentWeight(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UInventoryTestAttributeSet, CurrentWeight, OldValue);
}
