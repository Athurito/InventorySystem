// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "InventorySelectionViewModel.generated.h"

class UInventoryItemViewModel;
/**
 * 
 */
UCLASS(BlueprintType)
class RPGINVENTORY_API UInventorySelectionViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()
    
public:
    // Initialisiert dieses ViewModel gegen den Inventory-Manager und einen konkreten Container-Index
    UFUNCTION(BlueprintCallable)
    void InitializeFromManager(class UInventoryManagerComponent* InManager, int32 InContainerIndex);

    // Setter für Auswahl (z. B. Klick im UI)
    UFUNCTION(BlueprintCallable)
    void SetSelectedItem(UInventoryItemViewModel* Item);

    // Layoutdaten, damit die UI frei gestalten kann
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 Rows = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify)
    int32 Cols = 0;

    // Ein ViewModel pro Slot
    UPROPERTY(BlueprintReadOnly, FieldNotify)
    TArray<TObjectPtr<UInventoryItemViewModel>> Slots;

    // Aktuell selektiertes Item im Container
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter="SetSelectedItem")
    TObjectPtr<UInventoryItemViewModel> SelectedItem;

private:
    // Schwache Referenz auf den Manager (Model)
    TWeakObjectPtr<class UInventoryManagerComponent> Manager;
    // Auf welchen Container dieses VM zeigt
    int32 ContainerIndex = INDEX_NONE;

    // Callback für Slot-Änderungen (kommt vom Manager)
    UFUNCTION()
    void OnManagerSlotChanged(int32 ChangedContainer, int32 SlotIndex);
};
