// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonTileView.h"
#include "CommonUserWidget.h"
#include "ContainerGrid.generated.h"

class UContainerSlotButton;
class URpg_ContainerComponent;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UContainerGrid : public UCommonUserWidget
{
	GENERATED_BODY()

private:
	virtual void NativeOnInitialized() override;

	TWeakObjectPtr<URpg_ContainerComponent> ContainerComponent;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonTileView> TileView = nullptr;

	// Entry-Klasse für jeden Slot (Button/Widget). Im Editor setzbar.
	UPROPERTY(EditDefaultsOnly, Category="Container|UI")
	TSubclassOf<UContainerSlotButton> SlotButtonClass;
};
