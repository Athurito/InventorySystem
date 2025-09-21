


#include "InteractionManagement/Widget/Rpg_HUDWidget.h"

#include "InteractionManagement/Rpg_InteractionComponent.h"
#include "InteractionManagement/Data/InteractableDataAsset.h"
#include "InteractionManagement/Widget/InteractPromptWidget.h"

void URpg_HUDWidget::BindToInteraction(URpg_InteractionComponent* InteractionComp)
{
	// 1) Altes Binding sauber lösen (wenn vorhanden)
	if (BoundInteraction)
	{
		BoundInteraction->OnInteractPromptChanged.RemoveDynamic(
			this, &URpg_HUDWidget::HandlePromptChanged);
		BoundInteraction = nullptr;
	}

	// 2) Neues Binding setzen (wenn vorhanden)
	if (InteractionComp)
	{
		InteractionComp->OnInteractPromptChanged.AddDynamic(
			this, &URpg_HUDWidget::HandlePromptChanged);
		BoundInteraction = InteractionComp;
	}

	// 3) UI-Grundzustand
	HideInteractPrompt();
}

void URpg_HUDWidget::HandlePromptChanged(bool bVisible, FInteractDisplayData Data)
{
	if (bVisible) ShowInteractPrompt(Data);
	else          HideInteractPrompt();
}

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
