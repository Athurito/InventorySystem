#include "InventoryManagement/Interact/Widget/Rpg_HUDWidget.h"
#include "InventoryManagement/Interact/Widget/InteractPromptWidget.h"
#include "InventoryManagement/Interact/Data/InteractableDataAsset.h"

void URpg_HUDWidget::ShowInteractPrompt(const FInteractDisplayData& Data)
{
	if (InteractPrompt)
	{
		InteractPrompt->SetPromptData(Data);
		InteractPrompt->SetPromptVisible(true);
	}
}

void URpg_HUDWidget::HideInteractPrompt()
{
	if (InteractPrompt)
	{
		InteractPrompt->SetPromptVisible(false);
	}
}
