// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "ContainerSlotButton.generated.h"

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
	int32 GetSlotIndex() const { return SlotIndex; }
	void SetSlotIndex(const int32 Index) { SlotIndex = Index; }
	
	int32 GetStackCount() const { return StackCount; }
	void SetStackCount(const int32 Count) { StackCount = Count; }
	
	FInv_InventoryEntry* GetInventoryItem() const { return InventoryItem; }

	void UpdateText() const;
	void UpdateIcon(UTexture2D* Icon) const;

private:
	int32 SlotIndex{INDEX_NONE};
	int32 StackCount{0};

	FInv_InventoryEntry* InventoryItem{nullptr};

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;
};
