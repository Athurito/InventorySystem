// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "PlayerInventory.generated.h"

class UCommonActivatableWidgetSwitcher;
class UInventoryTabButton;
class UTabList;
class UInventoryManagerComponent;
/**
 * 
 */
struct FContainerRef { TWeakObjectPtr<UInventoryManagerComponent> Comp; int32 Index=-1; };
UCLASS()
class RPGINVENTORY_API UPlayerInventory : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	UInventoryManagerComponent* GetPlayerContainerComponent() const { return PlayerContainerComponent.Get(); }
	
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	UInventoryManagerComponent* GetContextContainerComponent() const { return ContextContainerComponent.Get(); }
	
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	int32 GetContextContainerIndex() const { return ContextContainerIndex; }
	
private:
	virtual void NativeOnInitialized() override;
	void EnsurePlayerComponent();
	

	// Grid-Widget-Klasse (dein UContainerGrid)
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TSubclassOf<UUserWidget> ContainerGridClass = nullptr; // e.g., UContainerGrid
	
	TWeakObjectPtr<UInventoryManagerComponent> PlayerContainerComponent;
	TWeakObjectPtr<UInventoryManagerComponent> ContextContainerComponent;
	
	int32 ContextContainerIndex = INDEX_NONE;
	
	// Start-Tab (optional vom Aufrufer)
	int32 RequestedStartTabIndex = 0;

	// Mappings
	TMap<FName, FContainerRef> TabMap;
	TMap<FName, int32> TabToContentIndex; 
};
