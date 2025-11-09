// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "ContainerSlotButton.generated.h"

class URpg_ContainerComponent;

class UTextBlock;
struct FInv_InventoryEntry;
class UImage;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UContainerSlotButton : public UCommonButtonBase
{
	GENERATED_BODY()


public:
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	int32 GetSlotIndex() const { return SlotIndex; }
	void SetSlotIndex(const int32 Index) { SlotIndex = Index; }

	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	int32 GetStackCount() const { return StackCount; }
	void SetStackCount(const int32 Count) { StackCount = Count; }
	
	FInv_InventoryEntry* GetInventoryItem() const { return InventoryItem; }

	void UpdateText() const;
	void UpdateIcon(UTexture2D* Icon) const;

	// Drag&Drop/Context helpers for Blueprints
 UFUNCTION(BlueprintPure, Category="Inventory|UI")
	URpg_ContainerComponent* GetOwningContainerComponent() const { return OwningContainerComponent; }
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	int32 GetOwningContainerIndex() const { return OwningContainerIndex; }
	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	void InitializeSlotContext(URpg_ContainerComponent* InComponent, int32 InContainerIndex) { OwningContainerComponent = InComponent; OwningContainerIndex = InContainerIndex; }

private:
	int32 SlotIndex{INDEX_NONE};
	int32 StackCount{0};

	FInv_InventoryEntry* InventoryItem{nullptr};

 // Owning container context for BP drag/drop
	TObjectPtr<URpg_ContainerComponent> OwningContainerComponent = nullptr;
	int32 OwningContainerIndex{INDEX_NONE};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;
};
