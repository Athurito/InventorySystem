// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "InventoryShell.generated.h"

class UPlayerInventory;
class UCommonTabListWidgetBase;
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
	// Ersetzt die kontextuelle UI (kann null sein, um sie auszublenden)
	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	void SetContextWidget(UWidget* InWidget);
	
protected:

	// Wird beim Aktivieren erneut angewandt, falls nötig
	virtual void NativeOnActivated() override;

private:
	void ApplyContextWidget() const;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UPlayerInventory> PlayerInventory = nullptr;

	// Root für dynamische Zusatz-UI (z. B. Overlay/SizeBox/VerticalBox)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UOverlay> DynamicContentRoot = nullptr;

	

	UPROPERTY(Transient)
	TObjectPtr<UWidget> ActiveContextWidget = nullptr;
};
