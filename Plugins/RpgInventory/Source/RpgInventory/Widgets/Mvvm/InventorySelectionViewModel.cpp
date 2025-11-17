// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySelectionViewModel.h"

void UInventorySelectionViewModel::SetSelectedItem(UInventoryItemViewModel* Item)
{
	if (!Item || SelectedItem == Item) return;
	
	SelectedItem = Item;
	
	if (UE_MVVM_SET_PROPERTY_VALUE(SelectedItem, Item))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedItem);
	}
}
