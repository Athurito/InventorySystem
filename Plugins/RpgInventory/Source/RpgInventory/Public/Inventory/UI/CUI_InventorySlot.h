#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CUI_InventorySlot.generated.h"

class UImage;
class UBorder;
class UInventoryGridWidget;
class URPGInventoryComponent;
class UDragDropOperation;

UCLASS(BlueprintType)
class RPGINVENTORY_API UInventorySlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	UInventorySlotWidget();

	void Init(UInventoryGridWidget* InOwnerGrid, int32 InSlotIndex, URPGInventoryComponent* InInventory);
	void Refresh();

	int32 GetSlotIndex() const { return SlotIndex; }

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	UPROPERTY(Transient)
	UInventoryGridWidget* OwnerGrid = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<URPGInventoryComponent> Inventory = nullptr;

	int32 SlotIndex = INDEX_NONE;
	int32 CachedEntryId = INDEX_NONE;

	// Minimal visuals
	UPROPERTY(Transient)
	UImage* IconImage = nullptr;
	UPROPERTY(Transient)
	UBorder* Border = nullptr;
};
