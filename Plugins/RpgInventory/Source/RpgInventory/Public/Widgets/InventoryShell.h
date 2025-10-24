// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "InventoryShell.generated.h"

class UContainerGrid;
class URpg_ContainerComponent;
class UWidget;
class UOverlay;

/**
 * InventoryShell hält dauerhaft das Player-Inventory sichtbar und zeigt dynamisch
 * zusätzlich eine kontextuelle UI (z. B. Storage-Container, Crafting etc.).
 */
UCLASS()
class RPGINVENTORY_API UInventoryShell : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	// Initialisiert die Shell. Player-Inventory wird immer angezeigt.
	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	void InitializeShell(URpg_ContainerComponent* PlayerContainer, int32 PlayerContainerIndex,
		URpg_ContainerComponent* ContextContainer, int32 ContextContainerIndex, UWidget* ContextWidget);

	// Variante: Nur Player-Bindung; Kontext wird später via SetContextWidget gesetzt
	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	void InitializeShellPlayerOnly(URpg_ContainerComponent* PlayerContainer, int32 InPlayerContainerIndex);

	// Ersetzt die kontextuelle UI (kann null sein, um sie auszublenden)
	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	void SetContextWidget(UWidget* InWidget);

	// Zugriff für BP
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	UContainerGrid* GetPlayerInventoryGrid() const { return PlayerInventoryGrid; }
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	URpg_ContainerComponent* GetPlayerContainerComponent() const { return PlayerContainerComponent.Get(); }
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	int32 GetPlayerContainerIndex() const { return PlayerContainerIndex; }
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	URpg_ContainerComponent* GetContextContainerComponent() const { return ContextContainerComponent.Get(); }
	UFUNCTION(BlueprintPure, Category="Inventory|UI")
	int32 GetContextContainerIndex() const { return ContextContainerIndex; }

protected:
	virtual void NativeOnInitialized() override;

	// Wird beim Aktivieren erneut angewandt, falls nötig
	virtual void NativeOnActivated() override;

private:
	void ApplyPlayerBinding();
	void ApplyContextWidget();

	// Immer sichtbares Player-Grid (via UMG gebunden)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UContainerGrid> PlayerInventoryGrid = nullptr;

	// Root für dynamische Zusatz-UI (z. B. Overlay/SizeBox/VerticalBox)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UOverlay> DynamicContentRoot = nullptr;

	TWeakObjectPtr<URpg_ContainerComponent> PlayerContainerComponent;
	int32 PlayerContainerIndex = INDEX_NONE;

	TWeakObjectPtr<URpg_ContainerComponent> ContextContainerComponent;
	int32 ContextContainerIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> ActiveContextWidget = nullptr;
};
