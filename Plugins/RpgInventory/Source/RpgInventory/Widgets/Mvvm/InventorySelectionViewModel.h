// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "InventorySelectionViewModel.generated.h"

class UInventoryItemViewModel;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInventorySelectionViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	
	void SetSelectedItem(UInventoryItemViewModel* Item);
	
	UPROPERTY(BlueprintReadOnly, FieldNotify, Setter)
	TObjectPtr<UInventoryItemViewModel> SelectedItem;
};
