// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "InventoryItemViewModel.generated.h"

class UInventoryItemInstance;
class UInventoryManagerComponent;

/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInventoryItemViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    void SetFromItemInstance(UInventoryItemInstance* Instance, int32 InQuantity);

    UFUNCTION(BlueprintCallable)
    void ClearSlot();

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    bool bIsEmpty = true;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TObjectPtr<UInventoryItemInstance> ItemInstance;

private:
    friend class UInventorySelectionViewModel;
    // Optionaler Kontext (wird gesetzt vom Selection-VM)
    TWeakObjectPtr<UInventoryManagerComponent> Manager;
    int32 ContainerIndex = INDEX_NONE;
    int32 SlotIndex = INDEX_NONE;
};
