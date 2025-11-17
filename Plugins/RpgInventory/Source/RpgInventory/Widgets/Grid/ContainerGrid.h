#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "ContainerGrid.generated.h"

struct FInventoryEntry;
class UContainerSlotButton;
class UInventoryManagerComponent;
class UUniformGridPanel;
struct FInv_InventoryEntry;


UCLASS()
class RPGINVENTORY_API UContainerGrid : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// Verknüpft dieses Grid mit einer ContainerComponent und einem ContainerIndex (z. B. 0 = Player-Inventory)
	UFUNCTION(BlueprintCallable, Category="Container|UI")
	void BindToContainer(UInventoryManagerComponent* InComponent, int32 InContainerIndex);

	// Liefert die in der Definition hinterlegte Zeilen-/Spaltenanzahl
	UFUNCTION(BlueprintPure, Category="Container|UI")
	int32 GetRows() const { return CachedRows; }
	UFUNCTION(BlueprintPure, Category="Container|UI")
	int32 GetCols() const { return CachedCols; }
	UFUNCTION(BlueprintPure, Category="Container|UI")
	int32 GetTotalSlots() const { return CachedRows * CachedCols; }

	// Optional: Zugriff auf die gebundene Komponente und den Index
	UFUNCTION(BlueprintPure, Category="Container|UI")
	UInventoryManagerComponent* GetBoundComponent() const { return ContainerComponent.Get(); }
	UFUNCTION(BlueprintPure, Category="Container|UI")
	int32 GetBoundContainerIndex() const { return ContainerIndex; }

protected:
	virtual void NativeDestruct() override;

private:
	void CacheFromDefinition();
	void RebuildGrid();
	void UnbindFromCurrent() const;
	UFUNCTION()
	void HandleEntryChanged(FGuid InstanceId, const FInventoryEntry& Entry);
	void BindDelegates();

	// Neu: Cache der Slot-Widgets nach Index
	UPROPERTY()
	TArray<TObjectPtr<UContainerSlotButton>> SlotWidgets;

	// Neu: Handler für präzise Slot-Änderungen (Delegate aus Component)
	UFUNCTION()
	void HandleSlotChanged(int32 ContainerIdx, int32 SlotIdx, FGuid InstanceId);

	// Neu: Hilfsfunktion für 1-Slot-Update
	void UpdateOneSlot(int32 SlotIdx);
	
	
	TWeakObjectPtr<UInventoryManagerComponent> ContainerComponent;
	int32 ContainerIndex = INDEX_NONE;
	int32 CachedRows = 0;
	int32 CachedCols = 0;

	// Root-Panel für das manuelle Grid-Layout (im UMG-Designer anlegen und binden)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> GridRoot = nullptr;

	// Entry-Klasse für jeden Slot (Button/Widget). Im Editor setzbar.
	UPROPERTY(EditDefaultsOnly, Category="Container|UI")
	TSubclassOf<UContainerSlotButton> SlotButtonClass;
};
