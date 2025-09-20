#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "InventoryWidget.generated.h"

class UInventoryGridWidget;
class URPGInventoryComponent;
class UPanelWidget;

/** Root inventory widget that contains the equipment slots area and the backpack grid. */
UCLASS(BlueprintType)
class RPGINVENTORY_API UInventoryWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	void SetInventory(URPGInventoryComponent* InInventory);

	// Optionally auto-wire at construct time
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory|UI")
	bool bAutoBindOnConstruct = true;

protected:
	virtual void NativeConstruct() override;

	// Designed in BP: Create a Widget Blueprint based on this class and add/rename these children accordingly.
	UPROPERTY(meta=(BindWidgetOptional))
	UInventoryGridWidget* BackpackGrid = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UPanelWidget* EquipmentPanel = nullptr;

private:
	UPROPERTY(Transient)
	TObjectPtr<URPGInventoryComponent> Inventory = nullptr;
};
