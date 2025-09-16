#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "InventoryManagement/Interact/Data/InteractableDataAsset.h"
#include "InventoryManagement/Interact/Widget/InteractPromptWidget.h"
#include "Rpg_HUDWidget.generated.h"

class UWidget;
class UInteractPromptWidget;

UCLASS()
class RPGINVENTORY_API URpg_HUDWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowInteractPrompt(const FInteractDisplayData& Data);

	UFUNCTION(BlueprintCallable, Category="HUD")
	void HideInteractPrompt();

protected:
	UPROPERTY(meta=(BindWidget)) UInteractPromptWidget* InteractPrompt = nullptr;
};
