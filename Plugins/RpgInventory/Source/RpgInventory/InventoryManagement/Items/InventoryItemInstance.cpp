// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemInstance.h"

#include "InventoryItemDefinition.h"
#include "Fragments/InventoryItemFragment.h"
#include "Net/UnrealNetwork.h"

UInventoryItemInstance::UInventoryItemInstance()
{
}

void UInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, ItemDef);
}

void UInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void UInventoryItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 UInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool UInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

const UInventoryItemFragment* UInventoryItemInstance::FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}

void UInventoryItemInstance::SetItemDef(TSubclassOf<UInventoryItemDefinition> InDef)
{
	ItemDef = InDef;
}
