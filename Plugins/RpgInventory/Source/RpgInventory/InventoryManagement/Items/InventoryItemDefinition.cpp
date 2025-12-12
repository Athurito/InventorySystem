// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemDefinition.h"

#include "Fragments/InventoryItemFragment.h"




UInventoryItemFragment* UInventoryItemDefinition::FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}
