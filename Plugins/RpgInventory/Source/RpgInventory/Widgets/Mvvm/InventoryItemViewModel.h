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
    void SetFromItemInstance(UInventoryItemInstance* Instance, UInventoryManagerComponent* InManager, int32 InContainerIndex, int32 InSlotIndex);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    int32 GetSlotIndex() const { return SlotIndex; }
    UFUNCTION(BlueprintCallable, Category="Inventory")
    int32 GetContainerIndex() const { return ContainerIndex; }
    UFUNCTION(BlueprintCallable, Category="Inventory")
    UInventoryManagerComponent* GetManager() const { return Manager.Get(); }
    
    UFUNCTION(BlueprintCallable)
    void ClearSlot();

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    bool bIsEmpty = true;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TObjectPtr<UInventoryItemInstance> ItemInstance;
    
    
    // Ab hier: direkt bindbare UI-Daten
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    FText Description;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TSoftObjectPtr<UTexture2D> BackgroundIcon;
    
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 DurabilityCurrent = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 DurabilityMax = 0;
    
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 CurrentStackCount = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 MaxStackSize = 0;

private:
    friend class UInventorySelectionViewModel;
    // Optionaler Kontext (wird gesetzt vom Selection-VM)
    TWeakObjectPtr<UInventoryManagerComponent> Manager;
    int32 ContainerIndex = INDEX_NONE;
    int32 SlotIndex = INDEX_NONE;
    
    void FillFromDefinition();
    void FillFromStatTags();
    void FillFromContainerDefinition();
};
