// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "InventoryShell.generated.h"

class UWidget;
class UOverlay;

UCLASS()
class RPGINVENTORY_API UInventoryShell : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	// Ersetzt die kontextuelle UI (kann null sein, um sie auszublenden)
	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	void SetContextWidget(UWidget* InWidget);
	
protected:

	UFUNCTION(BlueprintCallable, Category="Inventory|UI")
	UWidget* GetPlayerInventory() const { return PlayerInventory; }
	// Wird beim Aktivieren erneut angewandt, falls nötig
	virtual void NativeOnActivated() override;

private:
	void ApplyContextWidget() const;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidget> PlayerInventory = nullptr;

	// Root für dynamische Zusatz-UI (z. B. Overlay/SizeBox/VerticalBox)
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UOverlay> DynamicContentRoot = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UWidget> ActiveContextWidget = nullptr;
};
