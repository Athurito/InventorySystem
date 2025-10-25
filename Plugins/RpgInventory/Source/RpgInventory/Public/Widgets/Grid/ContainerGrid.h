#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "ContainerGrid.generated.h"

class UContainerSlotButton;
class URpg_ContainerComponent;
class UUniformGridPanel;
/**
 * Ein generisches Grid-Widget, das eine ContainerComponent repräsentiert.
 * Baut die eigentliche Darstellung nicht hart im Code auf, sondern stellt
 * Bind/Metadata-Funktionen bereit, damit Blueprints flexibel die UI anlegen können.
 */
UCLASS()
class RPGINVENTORY_API UContainerGrid : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// Verknüpft dieses Grid mit einer ContainerComponent und einem ContainerIndex (z. B. 0 = Player-Inventory)
	UFUNCTION(BlueprintCallable, Category="Container|UI")
	void BindToContainer(URpg_ContainerComponent* InComponent, int32 InContainerIndex);

	// Liefert die in der Definition hinterlegte Zeilen-/Spaltenanzahl
	UFUNCTION(BlueprintPure, Category="Container|UI")
	int32 GetRows() const { return CachedRows; }
	UFUNCTION(BlueprintPure, Category="Container|UI")
	int32 GetCols() const { return CachedCols; }
	UFUNCTION(BlueprintPure, Category="Container|UI")
	int32 GetTotalSlots() const { return CachedRows * CachedCols; }

	// Optional: Zugriff auf die gebundene Komponente und den Index
	UFUNCTION(BlueprintPure, Category="Container|UI")
	URpg_ContainerComponent* GetBoundComponent() const { return ContainerComponent.Get(); }
	UFUNCTION(BlueprintPure, Category="Container|UI")
	int32 GetBoundContainerIndex() const { return ContainerIndex; }

protected:
	virtual void NativeOnInitialized() override;

private:
	void CacheFromDefinition();
	void RebuildGrid();

	TWeakObjectPtr<URpg_ContainerComponent> ContainerComponent;
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
