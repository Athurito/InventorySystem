#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CUI_InventoryGrid.generated.h"

class UUniformGridPanel;
class UUniformGridSlot;
class UInventorySlotWidget;
class URPGInventoryComponent;
class UUserWidget;

UCLASS(BlueprintType)
class RPGINVENTORY_API UInventoryGridWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	UInventoryGridWidget();

	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	void SetInventory(URPGInventoryComponent* InInventory);

	// Layout
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout")
	int32 Columns = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Layout")
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

	// Rebuilds grid children to match inventory capacity and state
	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	void RebuildGrid();

	// Called by slots when a drop occurs
	bool HandleDropOnSlot(int32 FromSlot, int32 ToSlot);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnInventoryChanged();

	void EnsureGridPanel();
	void RefreshSlots();

private:
	UPROPERTY(Transient)
	UUniformGridPanel* GridPanel = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<URPGInventoryComponent> Inventory = nullptr;
};
