#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Rpg_HUDWidget.generated.h"

struct FInteractDisplayData;
class URpg_InteractionComponent;
class UWidget;
class UInteractPromptWidget;
class UCommonActivatableWidgetStack;
class UCommonActivatableWidget;

UCLASS()
class RPGINVENTORY_API URpg_HUDWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	
	// Vom PlayerController/Character nach dem Erstellen aufrufen:
	UFUNCTION(BlueprintCallable, Category="HUD")
	void BindToInteraction(URpg_InteractionComponent* InteractionComp);

	// Diese bleiben als interne Helfer öffentlich
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowInteractPrompt(const FInteractDisplayData& Data);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void HideInteractPrompt();

	// Inventory/UI Stack Steuerung (CommonUI)
	UFUNCTION(BlueprintCallable, Category="HUD|Inventory")
	void PushInventoryContextClass(TSubclassOf<UCommonActivatableWidget> WidgetClass);
	UFUNCTION(BlueprintCallable, Category="HUD|Inventory")
	void PushInventoryContextInstance(UCommonActivatableWidget* WidgetInstance);
	UFUNCTION(BlueprintCallable, Category="HUD|Inventory")
	void PopInventoryContext();
	UFUNCTION(BlueprintCallable, Category="HUD|Inventory")
	void ClearInventoryContext();

protected:
	UPROPERTY(meta=(BindWidget)) UInteractPromptWidget* InteractPrompt = nullptr;
	// Optionaler Stack, im UMG platzieren und binden
	UPROPERTY(meta=(BindWidgetOptional)) UCommonActivatableWidgetStack* InventoryStack = nullptr;
	// Event-Handler
	UFUNCTION()
	void HandlePromptChanged(bool bVisible, FInteractDisplayData Data);
	// <- wichtig: bisher gebundene Komponente merken, um korrekt zu entbinden
	UPROPERTY() URpg_InteractionComponent* BoundInteraction = nullptr;

};
