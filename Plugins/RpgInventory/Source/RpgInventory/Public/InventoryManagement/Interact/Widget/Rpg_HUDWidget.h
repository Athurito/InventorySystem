#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "InventoryManagement/Interact/Data/InteractableDataAsset.h"
#include "InventoryManagement/Interact/Widget/InteractPromptWidget.h"
#include "Rpg_HUDWidget.generated.h"

class URpg_InteractionComponent;
class UWidget;
class UInteractPromptWidget;

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

protected:
	UPROPERTY(meta=(BindWidget)) UInteractPromptWidget* InteractPrompt = nullptr;
	// Event-Handler
	UFUNCTION()
	void HandlePromptChanged(bool bVisible, FInteractDisplayData Data);
	// <- wichtig: bisher gebundene Komponente merken, um korrekt zu entbinden
	UPROPERTY() URpg_InteractionComponent* BoundInteraction = nullptr;

};
